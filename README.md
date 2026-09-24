# MFTGuard

## Automated detection of potential NTFS timestamp manipulation through $MFT artifact analysis

[T1070.006 - Indicator Removal: Timestomp](https://attack.mitre.org/techniques/T1070/006/)

MFTGuard is a C-based DFIR tool designed to identify NTFS Master File Table records exhibiting signs of anomalous timestamp behavior. To accomplish this, MFTGuard parses an entire $MFT binary, record-by-record. As each record is being read, the various attribute timestamp values are compared against pre-defined rulesets. If a record is flagged as potentially suspect, that candidate record is stored in a hash table for further analysis. Finally, a structured JSON report is created that contains records that warrant further investigation as well as detailed statistics.

The goal of this project is not to automatically declare a file as a definite result of time stomping. Instead, MFTGuard is an investigative triage tool: it reduces the number of MFT records an investigator needs to examine and provides structured evidence that can be correlated with independent forensic artifacts. This not only saves time and resources, it also allows investigators to perform much deeper analysis of MFT records that are sure to require it.

## Installation & Usage

## Key Features

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