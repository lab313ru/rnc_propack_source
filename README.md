# rnc_propack_source
Decompiled source of the famous RNC ProPack compression tool

## Usage
- Unpack: \<u\> \<infile.bin\> [outfile.bin] [-i=hex_offset_to_read_from] [-k=hex_key_if_protected]
- Search: \<s\> \<infile.bin\>
- Seach&Extract: \<e\> \<infile.bin\>
- Pack: \<p\> \<infile.bin\> [outfile.bin] \<-m=1|2\> [-k=hex_key_to_protect]

### Specific games
-  Earthworm Jim, Alladdin: must use **-d=0x4000** for a correct packed data result.
