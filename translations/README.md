# Italian supplemental draft

`it.json` completes the text not supplied by compatible records in Rulesless's
Banjo-Tooie v0.9 patch. The draft was prepared with Google Translate, project
terminology and manual corrections. It still needs editorial and in-game review.
It is a project addition; do not attribute these additional translations to Rulesless.

Entries use the SHA-256 of the original USA record bytes before the first NUL.
Identical source strings share one entry. `translate` entries contain uppercase
Italian text. `preserve` entries document names, personal credits, functional
codes, numeric content or intentionally fictional speech. `review: project correction`
identifies individually corrected entries, not approval of every other string.

Use real Unicode accents in the catalog. The builder converts them to the native
font codes. Keep `{80}`–`{87}` button tokens and `~` substitutions, including repeated
tokens. Preserve line breaks and `#` markers. Do not shorten text by truncation:
the encoded text plus its NUL must fit a 255-byte native record.

`tools/build_italian_bundle.py` runs offline with the catalog, the private Rulesless
BPS and verified private USA ROM inputs. Its private report maps each catalog key
to asset/group/record coordinates and source bytes. Rebuild with the command in
`docs/specs/Tooie/language-packs.md`, then inspect the report and native layout.
Do not commit the source ROM, patched ROM, downloaded installer or reference pack.

Attribution and terms summary: `assets/languages/NOTICE-Italian.txt`.
