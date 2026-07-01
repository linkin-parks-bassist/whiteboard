#ifndef WB_MATH_H_
#define WB_MATH_H_

typedef struct wb_math_formula wb_math_formula;

wb_math_formula *wb_math_parse(const char *src);
void wb_math_free(wb_math_formula *formula);
void wb_math_measure(wb_math_formula *formula, float size, float *w, float *h, float *baseline);
void wb_math_draw(uint8_t *buf, wb_math_formula *formula, float x, float baseline_y, float size, uint32_t colour);
void wb_set_math_jitter_strength(float strength);
void wb_math_draw_seeded(uint8_t *buf, wb_math_formula *formula, float x, float baseline_y, float size, uint32_t colour, int seed);

#endif
