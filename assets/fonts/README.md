# Menu font

The GUI looks for `holidays-homework.ttf` in this directory. When it is present,
it is loaded once through ResourceManager and used for frontend text, Settings,
and GUI text that does not already select another font. The title uses
`assets/ui/title.png` independently of the font.

If the file is absent, raylib's default font is used so the client still runs.
To test the chosen font locally, download Holidays Homework, extract the TTF and
name it `holidays-homework.ttf` here. Restart the client after changing the file.

Source: https://www.dafont.com/holidays-homework.font
Author: imagex.

The author's page states that the font is free for personal use and asks users to
contact the author for commercial use. This repository does not include the font
file or claim permission to redistribute it. Confirm redistribution permission
and include the applicable license before committing a font binary.

The connection form currently accepts printable ASCII player names and IPv4
addresses. The loaded font uses the resource manager's existing default glyph set.
