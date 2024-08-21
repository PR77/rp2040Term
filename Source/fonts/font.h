#ifndef FONT_H
#define	FONT_H

#define GLYPH_COUNT             256
#define	GLYPH_WIDTH             8
#define	GLYPH_HEIGHT            16

typedef uint8_t                 st_glyphData[GLYPH_COUNT][GLYPH_HEIGHT];

typedef struct {
    const char *                fontName;
    const st_glyphData * const  glyphData;
    uint16_t                    glyphCount;
    uint8_t                     glyphWidth;
    uint8_t                     glyphHeigth;
} st_fontEntry;

#endif // FONT_H