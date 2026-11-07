# Music pack mapping tools

Maintainer scripts that produced `tools/audio/banjo-tooie-hd.json`, the map from an HD soundtrack album to the game's
own sequences, channel masks and area tempos. They read private inputs only (your album, your decompressed ROM image)
and write no audio. Requirements: Python 3.11+, NumPy, `ffmpeg` on `PATH`.

Environment: `TOOIE_ALBUM` (folder with the album MP3s and its `download-manifest.json`), `TOOIE_ROM_IMAGE` (the
decompressed USA image, 35098224 bytes).

1. `make_plan.py plan.tsv` lists every area preset (mask and tempo) and every other sequence.
2. Record the native music in game: `TOOIE_TEST_AUDIO_RENDER=plan.tsv`, start the game and go to Game Select (see
   [modding-and-extensions](../../../docs/specs/Tooie/modding-and-extensions.md)). Each entry becomes a WAV.
3. `features.py <cache> <album>/*.mp3` caches chroma features of the album.
4. `matrix.py <render-dir> <cache> <out>` scores every recording against every album file (chroma, best lag);
   `jingles.py <out.tsv> <render-dirs>` matches short sounds by onset-aligned spectra.
5. `loops.py <album>/*.mp3` finds each file's loop (longest repeat, refined on the waveform).
6. `assemble.py mapping.json report.tsv` joins the matches: area layers by (sequence, mask, tempo), whole songs and
   jingles; `refine.py` and `restart.py` adjust each loop on the waveform (the audio after the loop end must repeat
   the audio after the loop start).
7. Build a pack: `python tools/build_audio_pack.py <album> out.zip`. Check it in game with
   `TOOIE_TEST_AUDIO_RENDER_MOD=1` (records the final mix) and `verify_e2e.py`.

Some decisions are recorded in `assemble.py`: files that start alike were told apart with longer recordings, one
jingle by elimination, and preset `0x46` (Jinjo Village drained) was recorded separately. The scripts were written for
one run and are kept so the mapping can be reproduced and extended, not as a polished tool.
