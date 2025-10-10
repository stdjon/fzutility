<style>
    body{
        font-family: "Georgia", "Times New Roman";
        color: #dfdfdf;
        background: #221100;
        code {
            color: #9f9fff;
        }
    }
</style>
## The `fzutility` tool

By default, the `fzutility` tool converts Casio FZ-1 binary data files to the FZ-ML format, or vice versa.

### Examples

```
  fzutility bank.fzb
```

would produce `bank.fzml` as output.


```
fzutility data.fzml
```

would produce a binary file with an extension matching the data contained in the source (e.g. `data.fzv` for voice data).
