# Building

1. Set up the build:
    ```sh
    meson setup . build
    ```
2. Build the library:
    ```sh
    meson compile -C build
    ```

## Running the example archiver

```sh
./build/test/test-file --decompress <file.ras>
```

Compressing (and other write operations) is not (yet?) implemented.

# File format

Caveat:
the primary goal of this document is to familiarize the reader with the details required for reading and/or writing archives, compatible with Max Payne games.
Some of the features appear to not be supported even by the original RasMaker and thus might never be properly documented, if at all.

All integer and floating-point values are little-endian unless otherwise noted, the latter of which you might want to consider reading as an integer on non-x86 platforms.
<opinion>Storing versions as floats is stupid, anyway.</opinion>

## Overview

![Overview](/format.png)

## Header

| Offset | Length | Purpose                                                                            |
| ------ | ------ | ---------------------------------------------------------------------------------- |
| `00`   | 4      | Magic<sup>[1](#header_footnote1)</sup>                                             |
| `04`   | 4      | Encryption seed<sup>[2](#header_footnote2)</sup>                                   |
| `08`   | 4      | File count                                                                         |
| `0C`   | 4      | Directory count                                                                    |
| `10`   | 4      | File table size                                                                    |
| `14`   | 4      | Directory table size                                                               |
| `18`   | 4      | Archive version<sup>[3](#header_footnote3)</sup> <sup>[4](#header_footnote4)</sup> |
| `1C`   | 4      | Header checksum<sup>[5](#header_footnote5)</sup>                                   |
| `20`   | 4      | File table checksum<sup>[5](#header_footnote5)</sup>                               |
| `24`   | 4      | Directory table checksum<sup>[5](#header_footnote5)</sup>                          |
| `28`   | 4      | Format version<sup>[6](#header_footnote6)</sup>                                    |

<a name="header_footnote1">1</a>.
`52 41 53 00` or `RAS\0` in ASCII.  
<a name="header_footnote2">2</a>.
Decryption code in C:

```c
void
decode (size_t  count,
        uint8_t buffer[static count],
        int32_t seed)
{
    if (seed == 0)
    {
        seed = 1;
    }

    for (size_t i = 0; i < count; i++)
    {
        seed = (seed * 171) - ((seed / 177) * 30269);

        buffer[i] = (buffer[i] << (i % 5)) | (buffer[i] >> (8 - (i % 5)));
        buffer[i] = (buffer[i] ^ ((i + 3) * 6)) + (seed & 0xFF);
    }
}
```

<a name="header_footnote3">3</a>.
Encoded in binary32.  
<a name="header_footnote4">4</a>.
Corresponds with the version of the archiver and is used for compatibility reasons.
MAX-FX tools and Max Payne 2 modification tools ship RasMaker 1.2, so expect `3F99999A`.  
<a name="header_footnote5">5</a>.
CRC-32
(x<sup>32</sup> + x<sup>26</sup> + x<sup>23</sup> + x<sup>22</sup> + x<sup>16</sup> + x<sup>12</sup> + x<sup>11</sup> + x<sup>10</sup> + x<sup>8</sup> + x<sup>7</sup> + x<sup>5</sup> + x<sup>4</sup> + x<sup>2</sup> + x + 1)  
<a name="header_footnote6">6</a>.
`3` for the original Max Payne archives, `4` for Max Payne 2 archives.

## File table
### Entry

| Length   | Purpose                                           |
| -------- | ------------------------------------------------- |
| Variable | Name                                              |
| 4        | File size                                         |
| 4        | File entry size                                   |
| 4        | ?                                                 |
| 4        | Parent directory index                            |
| 4        | ?                                                 |
| 4        | Compression method<sup>[1](#file_footnote1)</sup> |
| 16       | Creation time<sup>[2](#file_footnote2)</sup>      |

<a name="file_footnote1">1</a>. Possible values:
```
1 - compressed
2 - ? (possibly a dummy value, identical to 3, in theory)
3 - stored
```
<a name="file_footnote2">2</a>.
Stored as a [SYSTEMTIME](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724950.aspx).

## Directory table
### Entry

| Length   | Purpose                                                                    |
| -------- | -------------------------------------------------------------------------- |
| Variable | Full path                                                                  |
| 16       | Creation time<sup>[1](#dir_footnote1)</sup> <sup>[2](#dir_footnote2)</sup> |

<a name="dir_footnote1">1</a>.
Stored as a [SYSTEMTIME](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724950.aspx).  
<a name="dir_footnote2">2</a>. With two exceptions:  
1. the root directory has all fields set to 0;  
2. the source directory (when not using -p) has the creation time of 1601-01-01 00:00:00.000.

## File data
### Stored file entry

Uncompressed entries only contain the individual file data.

### Compressed file entry

Compressed entries, along with the individual file data, also contain information
about the file size. They are recognizable by the prefix `RA->`.

| Length | Purpose                                  |
| ------ | ---------------------------------------- |
| 4      | Identifier<sup>[1](#cmp_footnote1)</sup> |
| 4      | File size                                |
| 4      | Compressed file size                     |
|        | Compressed data                          |

<a name="cmp_footnote1">1</a>.
`52 41 2D 3E` or `RA->` in ASCII.  

#### Compression method

[LZSS](https://dl.acm.org/doi/10.1145/322344.322346)

### Encrypted file entry

Encrypted entries don’t exist in the wild. They are recognizable by the prefix `RC->`.
