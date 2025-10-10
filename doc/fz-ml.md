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
## Outline of an FZ-ML document (`.fzml` file)

An FZ-ML document is intended to represent performance data from a Casio FZ-1 sampler in a human-readable and/or manipulable format. This data is obtained by analyzing existing MIDI dump files (with extensions `.fzb`, `.fbe`, `.fzf` or `.fzv`). These files typically hold a number of bank, voice and wave blocks corresponding to a the type of dump file in question. An effect block may also be included. An FZ-ML document will therefore contain data that corresponds to the originating binary file (and can even be used to reconstruct the original (or modified) `.fz[befv]` file if desired).

### The Root Node

The root of a `.fmzl` document will be one single `<fz-ml/>` node. This node should have two attributes:

* `version`: currently `0.1α`
* `file_type`: indicating the specifics of the originating source (`0` → `.fzf`, `1` → `.fzv`, `2` → `.fzb`, `3` → `.fze`).

### Child Nodes

The root node should contain (in this order):

1. Zero or one `<effect/>` nodes
2. Zero or more `<bank/>` nodes
3. Zero or more `<voice/>` nodes
4. Zero or more `<wave/>` nodes

While this description doesn't preclude an empty root node, that doesn't seem like a very interesting use case.

### `<effect>` Node

An optional node that contains "effect" (global) data from an effect (`.fze`) of full (`.fzf`) dump. Cannot appear in bank/voice dumps, and having multiple instances would be meaningless.
Effect nodes can contain up to 24 attributes, but only attributes with a
non-zero value are recorded in the `.fzml` file:

* `pitchbend_depth`: Pitchbend depth, in 1/16th tone steps.
* `master_volume`: Master volume.
* `sustain_switch`: **TBD**
* `modulation_lfo_pitch`: **TBD**
* `modulation_lfo_amplitude`: **TBD**
* `modulation_lfo_filter`: **TBD**
* `modulation_lfo_filter_q`: **TBD**
* `modulation_filter`: **TBD**
* `modulation_amplitude`: **TBD**
* `modulation_filter_q`: **TBD**
* `footvolume_lfo_pitch`: **TBD**
* `footvolume_lfo_amplitude`: **TBD**
* `footvolume_lfo_filter`: **TBD**
* `footvolume_lfo_filter_q`: **TBD**
* `footvolume_amplitude`: **TBD**
* `footvolume_filter`: **TBD**
* `footvolume_filter_q`: **TBD**
* `aftertouch_lfo_pitch`: **TBD**
* `aftertouch_lfo_amplitude`: **TBD**
* `aftertouch_lfo_filter`: **TBD**
* `aftertouch_lfo_filter_q`: **TBD**
* `aftertouch_amplitude`: **TBD**
* `aftertouch_filter`: **TBD**
* `aftertouch_filter_q`: **TBD**

### `<bank>` Nodes

Contain the description of a Bank. Each `<bank>` node has three attributes:

* `name`: the bank's name. 12 characters followed by two zero bytes. Banks generated on the FZ-1 use space characters to right-pad names with fewer than 12 characters.
* `index`: the bank's index (ranging from 0-7).
* `voice_count`: the number of Voice definitions associated with the bank.

In addition to these attributes, each bank contains nine sub-nodes. Each sub-node contains a comma-separated list of values, one value for each voice parameter, as indicated by the `voice_count` attribute:

* `<midi_hi>`, `<midi_hi>`: High and low MIDI note limits for the voice (ranging from 0-127).
* `<velocity_hi>`, `<velocity_hi>`: High and low note velocity limits for the voice (ranging from 0-127).
* `<midi_origin>`: The MIDI note value for the origin (or root) note of the voice (ranging from 0-127).
* `<midi_channel>`: The MIDI channel that the voice responds to (ranging from 0-15).
* `<output_mask>`: A bitmask representing the individual (physical) outputs that the voice will be output to (ranging from 0-255, where 255 means "play on all outputs").
* `<area_volume>`: Volume of the voice? **NB: further description needed**
* `<voice_index>`: Index of the specific voice whose data will be used to play.

### `<voice>` Nodes

Contain Voice descriptions. Voice nodes have two mandatory attributes:

* `name`: the voice's name. 12 characters followed by two zero bytes. Voices generated on the FZ-1 use space characters to right-pad names with fewer than 12 characters.
* `index`: the voice's index (ranging from 0-63).

Voices can also have up to 35 additional attributes, but only attributes with non-zero values are recorded in the `.fzml` file:

* `data_start`: **TBD**
* `data_end`: **TBD**
* `play_start`: **TBD**
* `play_end`: **TBD**
* `loop`: **TBD**
* `loop_sustain_point`: **TBD**
* `loop_end_point`: **TBD**
* `pitch_correction`: **TBD**
* `filter`: **TBD**
* `filter_q`: **TBD**
* `dca_sustain`: **TBD**
* `dca_end`: **TBD**
* `dcf_sustain`: **TBD**
* `dcf_end`: **TBD**
* `lfo_delay`: **TBD**
* `lfo_name`: **TBD**
* `lfo_attack`: **TBD**
* `lfo_rate`: **TBD**
* `lfo_pitch`: **TBD**
* `lfo_amplitude`: **TBD**
* `lfo_filter`: **TBD**
* `lfo_filter_q`: **TBD**
* `velocity_filter_q_key_follow`: **TBD**
* `amplitude_key_follow`: **TBD**
* `amplitude_rate_key_follow`: **TBD**
* `filter_key_follow`: **TBD**
* `filter_rate_key_follow`: **TBD**
* `velocity_amplitude_key_follow`: **TBD**
* `velocity_amplitude_rate_key_follow`: **TBD**
* `velocity_filter_key_follow`: **TBD**
* `velocity_filter_rate_key_follow`: **TBD**
* `midi_hi`: **TBD**
* `midi_lo`: **TBD**
* `midi_origin`: **TBD**

Voices also contain sub-nodes, each of which will contain a comma-separated list containing eight values:

* `<loop_start>`, `<loop_end>`: Start and end points for the 8 loops.
* `<loop_xfade_time>`: Crossfade time for each loop.
* `<loop_time>`: Time taken by each loop.
* `<dca_rate>`, `<dca_end_level>`: DCA envelope rates and end-points.
* `<dcf_rate>`, `<dcf_end_level>`: DCF envelope rates and end-points.

### `<wave>` Nodes

Each Wave node has a single `index` attribute to indicate its place in the list of wave nodes/blocks. A `<wave>` node consists of 512 samples stored as 2-byte (16-bit) signed values.

`fzutility` writes each node's sample data out as four hexadecimal characters for each sample followed by a space: 32 rows of 16 samples each (followed by a newline). This is deemed to provide a balance between compactness and readability by humans.

When reading in wave data, a more permissive scheme is used: whitespace is stripped from the input until a non-whitespace character is found, at which point the next four characters are interpreted as a 16-bit hex value, the read cursor skips forward 4 characters and the process resumes. This allows for the possibility of other tools that want to format the wave data differently and still interoperate with `fzutility`.
