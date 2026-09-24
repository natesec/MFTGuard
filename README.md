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

The NTFS attributes list is walked and parsed for each record to extract the main metadata required for timestamp analysis. The parser currently supports `$STANDARD_INFORMATION` (`$SI`) and `FILE_NAME` (`$FN`) attribute types.

During the attribute walk, attribute lengths and boundaries are validated before reading the contents. Attribute headers and resident attribute headers are parsed separately using fixed offsets. For each supported attribute, MFTGuard extracts the relevant fields and stores them using dedicated little-endian readers. For example:
```C
si->creation_time = read_u64_le(value + 0x00);
```

For `$STANDARD_INFORMATION` attribute, only one per record is supported. The timestamp fields are parsed and stored in the `record_metadata` struct.

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

MFTGuard evaluates the timestamp metadata extracted from the `$STANDARD_INFORMATION` and `$FILE_NAME` attributes using a defined set of detection rules. These rules were honed using statistics on several MFT artifact samples extracted from virtual machines. Rather than treating a single timestamp discrepancy as definitive evidence of timestamp manipulation, the rules identify records exhibiting complex timestamp relationships that warrant further analysis. Rules are often evaluated in combination to eachother and with variable thresholds. A single record can trigger multiple rules.

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

This bitmask is then stored with the respective record for later candidate analysis and reporting.

### Statistical Analysis

Aggregate statistics are maintained throughout the parsing process, collecting information as each record is parsed. The overall composition and processing results of the input MFT are initially collected:

- Total records processed
- Records flagged for further investigation
- Unused records
- Reserved records
- Invalid records
- Records that could not be read or were skipped

Additionally, `$FILE_NAME` statistics are collected:

- Number of records containing `$FILE_NAME` attributes
- Number of records containing multiple `$FILE_NAME` attributes
- Total `$FILE_NAME` attributes encountered
- Maximum number of `$FILE_NAME` attributes in a single record
- Distribution of `$FILE_NAME` counts per record

For timestamp relationship statistics, the following counts are collected for both the `$STANDARD_INFORMATION` and `$FILE_NAME` timestamps for each field (creation, modified, MFT-modified, and accessed)

- Before corresponding `$FILE_NAME` value
- At the same time as the corresponding `$FILE_NAME` value
- After the corresponding `$FILE_NAME` value

Aggregate counts of which rules were triggered are also tracked.

The most involved processes in the statistics portion of this project were those relating to the timestamp deltas. For each corresponding SI/FN timestamp pair, MFTGuard calculates the difference between the two values and maintains aggregate statistics for the observed deltas. For each timestamp type, the following are included:

- Minimum observed delta
- Maximum observed delta
- Sum of observed deltas
- Number of observations
- Number of deltas exceeding 1 second
- Number exceeding 1 minute
- Number exceeding 1 hour
- Number exceeding 1 day
- Number exceeding 1 week
- Number exceeding 30 days

All of these measurements allow an investigator to examine the MFT artifact closely. These statistics are also useful when it comes to adjusting the thresholds and rules, generally reducing the false-positive rate. All of these statistics are included in the JSON report, providing both aggregate MFT-wide context and record-level forensic detail.

### Hash Table Candidate Management

MFTGuard utilizes the uthash library to create a hash table and subsequently to manage records identified by the rule set as candidates for further investigation. Candidates are keyed by their MFT record number so as to allow direct association between the stored candidate and its original MFT record.

Candidate collection occurs after rule evaluation. When a record satisfies `rules_should_report()`, MFTGuard creates a deep copy of all relevant record metadata and stores it in the hash table. The following information is preserved in each candidate for further reporting:

- MFT record number
- Rule flags
- `$STANDARD_INFORMATION` metadata
- All extracted `$FILE_NAME` attributes
- Associated filename data and metadata

The candidate is deep-copied rather than retaining pointers to temporary metadata since the same metadata structures are re-used as each record is processed. An independent copy of each flagged record also allows better MFT-wide analysis.

This hash table is later iterated over to produce the final JSON report. This approach allows MFTGuard to complete analysis without coupling candidate detection directly to report generation.

### JSON Reporting

The structured JSON report contains both aggregate MFT statistics and detailed information for records identified as candidates. For report generation, the cJSON library was used. As far as organization, the report is split into two main sections:

- Overview = aggregate information describing the MFT and the analysis performed, including record status counts, rule counts, `$FILE_NAME` statistics, SI/FN timestamp relationships, and timestamp delta statistics.
- Candidates = a massive array containing detailed metadata for each record that satisfied the reporting criteria, including its record number, triggered rule flags, `$STANDARD_INFORMATION` data, and all extracted `$FILE_NAME` attributes.

The overview and statistical data are generated first. Candidate records are then written incrementally as the candidate hash table is traversed. Rather than constructing a potentially enormous JSON array in memory, each candidate is serialized and written to the report individually.

This streaming approach is particularly important when processing large MFT artifacts. In testing, MFTGuard encountered more than 180,000 candidates, making it impractical to construct the entire candidate section as an in-memory cJSON object. Streaming the candidate output keeps memory consumption substantially lower while still producing a single valid JSON document.

Here is a simplified representation of the report structure:

```json
{
  "overview": {
    "records": {
      "processed": 489831,
      "flagged": 134177,
      "unused": 334869,
      "reserved": 16,
      "invalid": 2164,
      "skipped": 0
    },
    "rules": {
      "si_fn_mismatch": 76729,
      "timestamp_rollback": 183763,
      "zeroed_timestamp": 294,
      "identical_timestamp": 57154
    },
    "file_names": {
      "records": 485245,
      "multiple_records": 324105,
      "total": 837619,
      "max_per_record": 6
    },
    "timestamp_deltas": {
      "creation": {
        "min": -3052569920000000,
        "max": 134239274290688510,
        "count": 837619,
        "over_1s": 319542,
        "over_1d": 182088,
        "over_30d": 40031
      }
    }
  },
  "candidates": [
    {
      "record_number": 24,
      "rule_flags": 8,
      "has_standard_information": true,
      "standard_information": {
        "creation_time": "134239199197368063",
        "creation_time_utc": "2026-05-22T10:38:39Z"
      },
      "file_name_count": 1,
      "file_names": [
        {
          "parent_directory": 3096224743817227,
          "filename_namespace": "...",
          "filename": "filename.exe"
        }
      ]
    }
  ]
}
```

Timestamp values are represented using both their raw NTFS FILETIME value and a UTC ISO-8601 representation. The raw value preserves the original timestamp representation while the human-readable UTC value makes the report easier to inspect and correlate with other forensic timelines

The resulting report is designed to be easy for further scripts and tools to corroborate the scan with other forensic artifacts while maintaining readability. The output can be straightforward to inspect with tools such as `jq`.

### Performance

MFTGuard is designed to process large MFT artifacts efficiently while performing validation, attribute parsing, statistical analysis, rule evaluation, candidate collection, and structured report generation. For example: testing was performed using an MFT artifact obtained from a virtual machine containing 826,880 records. Both a clean sample and a test sample containing intentionally modified timestamps were processed.

The full analysis, including the JSON report generation, completed in approximately 10 seconds for a single 826,880-record artifact. This benchmark includes the complete workflow from execution.

The modified sample was created for controlled testing and is not intended to represent every possible real-world timestomping technique. With that being said, this benchmark demonstrates that MFTGuard can analyze hundreds of thousands of MFT records and produce a detailed forensic report without requiring a separate processing stage for each component.

## Lessons Learned

MFTGuard began with a much more ambitious goal: develop a timestomping detection tool that could identify suspicious timestamp manipulation with a very low false-positive rate using nothing but the $MFT binary by itself. As development progressed and the tool was tested against real MFT data, this proved to be unrealistic.

### The MFT Contains Significant Noise

NTFS metadata naturally contains a large amount of variation. Differences in `$FILE_NAME` and `$STANDARD_INFORMATION` timestamps can occur for many reasons that aren't at all related to malicious timestamp manipulation. Treating individual timestamp discrepancies as evidence of timestomping produced far too many candidates.

To reduce the false-positive rate, I initially explored increasingly restrictive combinations of timestamp relationships and statistical thresholds in an attempt to reduce this noise. Through testing, the estimated false-positive rate was reduced from approximately 22% to 16%. While this represented an improvement in the behavior of the detection rules, it also demonstrated an important limitation: increasingly complex rules applied to the MFT alone could not reliably distinguish malicious manipulation from legitimate filesystem behavior.

This changed the direction of the project. Instead of trying to detect timestomping, the tool was redesigned around forensic triage and evidence correlation.

### Correlation Is Essential

The most promising use of MFTGuard came from treating the output as a starting point for investigation rather than a one-stop-shop. The resulting report provides detailed candidate records, timestamp relationships, rule flags, filenames, and aggregate statistics that can be correlated with independent forensic artifacts. Sources such as the USN Journal, $LogFile, Prefetch, Amcache, ShimCache, Windows event logs, Sysmon, LNK files, Jump Lists, and application-specific logs can provide additional context about what happened around the timestamps identified by MFTGuard.

The final design philosophy of the project is:
> MFTGuard points the way, independent forensic artifacts reveal what actually happened

The biggest lesson from the project was therefore not how to create a more complicated detection rule. It was learning where the available evidence stops being sufficient and designing the tool to work effectively within that limitation.

## Future Development

## Disclaimer

## Libraries

### cJSON

### uthash

## License