# MFTGuard

## Automated detection of potential NTFS timestamp manipulation through $MFT artifact analysis

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
MFTGuard.exe <$mft> <sector size>
```

The default Windows sector size is 512. This will be suitable for most Windows machines.

After a successful scan, a JSON report file is created with the filename `mftguard_report.json`.

## Key Features

MFTGuard is a complete pipeline:
Raw filesystem artifact -> binary parsing -> NTFS fixup -> attribute parsing & extraction -> timestamp analysis -> statistical analysis -> candidate management -> structured forensic reporting -> independent artifact correlation

### MFT Parsing

MFTGuard parses the NTFS Master File Table directly from its binary representation, processing the MFT record-by-record rather than relying on a third-party MFT parser for its core analysis.

### NTFS Fixup / Update Sequence Arrays

### Attribute Parsing

### Timestamp Analysis

### Statistical Analysis

### Hash Table Candidate Management

### JSON Reporting

### Performance

## Technical Details

## Correlating MFTGuard Findings

### 1. USN Journal

### 2. NTFS $LogFile

### 3. Windows & Application Artifacts

## Current Limitations & Future Development

## Disclaimer

## Libraries

### cJSON

### uthash

## License