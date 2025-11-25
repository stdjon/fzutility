## The `fzutility` tool

`fzutility` is intended as a tool for owners of the Casio FZ-1 sampler. It can convert existing binary files (obtained via MIDI) into an XML-based text format (FZ-ML, file extension `.fzml`), or vice versa.

For binary dumps or FZ-ML files that contain wave data, this can be extracted as `.wav` format files.

Some additional features may also be added to support e.g. building or modification of FZ-1 banks and voices on a computer.

### Default usage

By default, the `fzutility` tool converts Casio FZ-1 binary data files to the FZ-ML format, or vice versa.

```
fzutility ‹input› [‹output›]
```

reads the file specified by `‹input›` and produces an output file. If `‹output›` was specified, this will be the name of the output file, otherwise the name will match the input file with an extension that represents the file's type.

#### Examples

```
fzutility input.fzf output.1
```

would produce an FZ-ML file with the name `output.1` (exactly as specified by the command).

```
fzutility bank.fzb
```

would produce `bank.fzml` as output.


```
fzutility data.fzml
```

would produce a binary file with an extension matching the data contained in the source (e.g. `data.fzv` for voice data).

### File inspection

The `-i` option specifies inspection of a given binary or FZ-ML file:

```
fzutility -i ‹input›
```

inspects a file and prints out a list of the objects it contains (presence or absence of effect data, banks and voices with their names, and the number of wave blocks).

### Wave data extraction

The `-w` option can be used to extract wave data from a binary or FZ-ML file, should any exist:

```
fzutility -w ‹input› [‹range›] [‹output›]
```

Range specifiers are of the format `start-end` or `start~end`, with the default being `0-0`.

* For `start-end`, both `start` and `end` count samples into the available wave data e.g.: `0-100` is (up to) the first 100 samples.

* For `start~end`, the `end` value is counted from the _end_ of the sample, so `0~100` is everything but the last 100 samples of wave data.

If unspecified, the output filename is the `‹input›` with the file extension replaced with `.wav`.
