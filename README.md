# MFTGuard

## Automated Detection of Potential NTFS Timestamp Manipulation Through $MFT Artifact Analysis

[T1070.006 - Indicator Removal: Timestomp](https://attack.mitre.org/techniques/T1070/006/)

MFTGuard is a C-based DFIR tool designed to identify NTFS Master File Table records exhibiting signs of anomalous timestamp behavior. To accomplish this, MFTGuard parses an entire $MFT binary, record-by-record. As each record is being read, the various attribute timestamp values are compared against pre-defined rulesets. If a record is flagged as potentially suspect, that candidate record is stored in a hash table for further analysis. Finally, a structured JSON report is created that contains records that warrant further investigation as well as detailed statistics.

The goal of this project is not to automatically declare a file as a definite result of timestomping. Instead, MFTGuard is an investigative triage tool: it reduces the number of MFT records an investigator needs to examine and provides structured evidence that can be correlated with independent forensic artifacts. This not only saves time and resources, it also allows investigators to perform much deeper analysis of MFT records that are sure to require it.


## Installation & Usage

### Requirements

The following are required to build MFTGuard from source:
- C compiler with C11 support
- CMake

CMake is used to simplify the process of configuring and generating build files.

### Build

1. Clone the repository
```
git clone https://github.com/natesec/MFTGuard.git
```

2. Create a build directory
```
mkdir build
```

3. Configure the project and build with CMake
```
cd build
cmake ..
cmake --build .
```

The resulting executable will be located in the generated build directory.

### Input

MFTGuard takes a raw $MFT binary file as input. You can extract such a file easily with a tool such as [KAPE](https://www.kroll.com/en/publications/cyber/kroll-artifact-parser-extractor-kape). Once installed, you can use the following command on Windows to extract the $MFT:
```
kape.exe --tsource C: --target "$MFT" --tdest <output-path>
```

The resulting $MFT file can be provided to MFTGuard for analysis.

### Usage

You can use MFTGuard to analyze a raw binary $MFT file with:
```
MFTGuard.exe <$mft> <sector-size>
```

The default Windows sector size is 512. This will be suitable for most Windows machines.

After a successful scan, a JSON report file is created with the filename `mftguard_report.json`.

## Key Features

MFTGuard is a complete pipeline:
raw filesystem artifact -> binary parsing -> NTFS fixup -> attribute parsing & extraction -> timestamp analysis -> statistical analysis -> candidate management -> structured forensic reporting -> independent artifact correlation.

### MFT Parsing

MFTGuard parses the NTFS Master File Table directly from its binary representation, processing the MFT record-by-record rather than relying on a third-party MFT parser for its core analysis.

Before individual records are analyzed, MFTGuard determines the record sizes by reading the `allocated_size` field in the MFT header. By dividing the file size in bytes by the `allocated_size`, we can validate this and ensure the file can be divided into equal records. The current release supports the standard 1024-byte record sizes.

Each record is validated before its attributes are interpreted. Validation includes checks such as:

- Header size
- `FILE` and `BAAD` signatures
- USA offset and count
- Attribute list offset
- `used_size` vs `allocated_size`
- Record and file boundaries
- Other structural constaints required for safe record parsing

MFTGuard distinguishes record states with the `mft_record_status` enum as opposed to treating every record as either valid or invalid. Each record is classified according to the result of the parsing process:

| Status | Description |
|--------------------|--------------------|
| `MFT_RECORD_INVALID` | Record was read but malformed or corrupt |
| `MFT_RECORD_UNUSED` | Valid record but not in-use |
| `MFT_RECORD_RESERVED` | Reserved NTFS record (typically the first 16 records in the MFT) |
| `MFT_RECORD_SKIPPED` | Failed to read record |
| `MFT_RECORD_USED` | Eligible for rule evaluation |

These stats are tracked in the `statistics` struct and are later included in the final JSON report to provide further visibility.

### NTFS Fixup / Update Sequence Arrays

NTFS Update Sequence Array (USA) fixup processing is implemented to validate and restore sector-tail values within each record before their contents are interpreted.

For each record, this is the general fixup process:

1. Verifies that the record size is evenly divisible by the configured sector size
2. Calculates the expected number of sectors in the record
3. Validates that the USA count matches the expected sector count plus one
4. Validates that the complete USA fits within the record boundaries
5. Extracts the Update Sequence Number (USN) from `USA[0]`
6. Examines the final `WORD` of every sector and verifies that it matches the expected USN
7. Restores the original sector-tail `WORD` from the corresponding USA entry

If any of the previously-mentioned steps fail, the record is marked as `MFT_RECORD_INVALID` and not analyzed.

The fixup implementation operates directly on the MFT record buffer, reconstructing the accurate record data for subsequent attribute analysis. This is because allocating a separate buffer for the reconstructed record was deemed unoptimal during development. An MFT can contain upwards of a million records, so unecessary memory operations can add up.

### Attribute Parsing

The NTFS attributes list is walked and parsed for each record to extract the main metadata required for timestamp analysis. The parser currently supports `SOURCE_INFORMATION` (`$SI`) and `FILE_NAME` (`$FN`) attribute types.

During the attribute walk, attribute lengths and boundaries are validated before reading the contents. Attribute headers and resident attribute headers are parsed separately using fixed offsets. For each supported attribute, MFTGuard extracts the relevant fields and stores them using dedicated little-endian readers. For example:
```C
si->creation_time = read_u64_le(value + 0x00);
```

For `SOURCE_INFORMATION` attribute, only one per record is supported. The timestamp fields are parsed and stored in the `record_metadata` struct.

For `FILE_NAME` attributes, there can be one, multiple, or none for each record. The distribution of `FILE_NAME` counts is collected in the statistics struct and later included in the JSON report. All instances present are stored in the `record_metadata` struct to preserve all relevant data for later timestamp analysis.

Here is an example of the `FILE_NAME` count distribution from my sample $MFT:

| `FILE_NAME` Count | Instances |
|-------------------|-----------|
| 0 | 4586 |
| 1 | 161140 |
| 2 | 302165 |
| 3 | 15910 |
| 4 | 5752 |
| 5 | 257 |
| 6+ | 21 |

As you can see, the vast majority of records have multiple `FN` attributes. Usually, this is because of hard links: a file can exist in multiple locations in a filesystem, each with separate fields.

### Timestamp Analysis

MFTGuard evaluates the timestamp metadata extracted from the `$SOURCE_INFORMATION` and `$FILE_NAME` attributes using a defined set of detection rules. These rules were honed using statistics on several MFT artifact samples extracted from virtual machines. Rather than treating a single timestamp discrepancy as definitive evidence of timestamp manipulation, the rules identify records exhibiting complex timestamp relationships that warrant further analysis. Rules are often evaluated in combination to eachother and with variable thresholds. A single record can trigger multiple rules.

For each parsed record, the rule functions evaluate the metadata and return a bitmask. An enum is used to store the flags of each rule in the form of a bit position (each rule undergoes a left bit shift according to its order in the enum). Once every rule has been evaluated, the flags undergo a bitwise `OR` operation to collapse into a single bitmask, retaining all rule-based detection data efficiently.

The current rule set evaluates:

- SI/FN Mismatch = variable number of`SI` creation times are after `FN`creation times
- Timestamp Rollback = variable number of `SI` modified times, mft_modified timest, or accessed_times are before the respective `FN` timestamps
- Zeroed Timestamps = variable number of timestamps in either the `SI` or `FN` attributes have variable anomalous decimal-point precision
- Identical Timestamps = any or all `SI` timestamp values are identical to their `FN` counterparts OR all `SI` timestamps are exactly equal

The rule logic combines triggered conditions using bitwise flags:
```c
#define RULE_FLAG(id) (1u << (id))
rule_flags |= RULE_FLAG(RULE_SI_FN_MISMATCH);
rule_flags |= RULE_FLAG(RULE_TIMESTAMP_ROLLBACK);
rule_flags |= RULE_FLAG(RULE_ZEROED_TIMESTAMP);
rule_flags |= RULE_FLAG(RULE_IDENTICAL_TIMESTAMPS);
```

This bitmask is then stored in the record metadata for later candidate analysis and reporting.

### Statistical Analysis

### Hash Table Candidate Management

### JSON Reporting

### Performance

## Lessons Learned

## Correlating MFTGuard Findings

### 1. USN Journal

### 2. NTFS $LogFile

### 3. Windows & Application Artifacts

## Future Development

## Disclaimer

## Libraries

### cJSON

### uthash

## License