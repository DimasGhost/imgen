#include "gen_result.cpp"
#include "../random_helper.cpp"

using namespace std;
using namespace json11;
using namespace cimg_library;


class GenPaintContext {

public:

	Image *image;
	RandomHelper rnd;
	int triangles_cnt;
	int axis_div;
	double opacity;

	GenPaintContext(Image *image, int triangles_cnt, int axis_div, double opacity):
		image(image), triangles_cnt(triangles_cnt), axis_div(axis_div), opacity(opacity) {}
};


class PaintTriangle {

private:

	void random_jump(int &x, int jump, int l, int r, GenPaintContext *context) {
		x = max(min(x + context->rnd.random_int(-jump, +jump), r), l);
	}

	int psign(int x1, int y1, int x2, int y2, int x3, int y3) {
		return (x1 - x3) * (y2 - y3) - (x2 - x3) * (y1 - y3);
	}

	bool point_in_triangle(int px, int py, int x1, int y1, int x2, int y2, int x3, int y3) {
		int d1, d2, d3;
	    bool has_neg, has_pos;

	    d1 = psign(px, py, x1, y1, x2, y2);
	    d2 = psign(px, py, x2, y2, x3, y3);
	    d3 = psign(px, py, x3, y3, x1, y1);

	    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	    return !(has_neg && has_pos);
	}

	void evaulate_color(GenPaintContext *context) {
		int xmin = min(min(x1, x2), x3);
		int xmax = max(max(x1, x2), x3);
		int ymin = min(min(y1, y2), y3);
		int ymax = max(max(y1, y2), y3);

		r = g = b = 0;
		int cnt = 0;

		Image *image = context->image;

		for (int i = xmin; i <= xmax; ++i)
			for (int j = ymin; j <= ymax; ++j)
				if (point_in_triangle(i, j, x1, y1, x2, y2, x3, y3)) {
					r += (*image)(i, j, 0, 0);
					g += (*image)(i, j, 0, 1);
					b += (*image)(i, j, 0, 2);
					++cnt;
				}
		r /= cnt;
		g /= cnt;
		b /= cnt;
	}

	int cx, cy, len1, len2, len3;
	double px1, py1, px2, py2, px3, py3;

	void eval_side(int len, double px, double py, int &nx, int &ny, int w, int h) {
		double mul = len / sqrt(px * px + py * py);
		nx = min(max(int(cx + px * mul), 0), w - 1);
		ny = min(max(int(cy + py * mul), 0), h - 1);
	}

	void recount(int w, int h) {
		eval_side(len1, px1, py1, x1, y1, w, h);
		eval_side(len2, px2, py2, x2, y2, w, h);
		eval_side(len3, px3, py3, x3, y3, w, h);
	}

public:

	static int get_cnt(GenPaintContext *context) {
		return context->triangles_cnt;
	}

	int x1, y1, x2, y2, x3, y3, r, g, b;

	PaintTriangle(GenPaintContext *context, int num) {

		int w = context->image->width();
		int h = context->image->height();
		int avg = (w + h) / 2;

		cx = context->rnd.random_int(0, w - 1);
		cy = context->rnd.random_int(0, h - 1);

		int maxlen = avg / context->axis_div;
		if (num < context->triangles_cnt / 10)
			maxlen = avg / context->axis_div * 4;

		len1 = context->rnd.random_int(1, maxlen);
		len2 = context->rnd.random_int(1, maxlen);
		len3 = context->rnd.random_int(1, maxlen);

		px1 = context->rnd.random_double(-1, 1);
		py1 = context->rnd.random_double(-1, 1);
		px2 = context->rnd.random_double(-1, 1);
		py2 = context->rnd.random_double(-1, 1);
		px3 = context->rnd.random_double(-1, 1);
		py3 = context->rnd.random_double(-1, 1);

		recount(w, h);

		evaulate_color(context);
	}

	void mutate(GenPaintContext *context, int num) {
		int w = context->image->width();
		int h = context->image->height();
		int avg = (w + h) / 2;

		random_jump(cx, avg / 20, 0, w - 1, context);
		random_jump(cy, avg / 20, 0, h - 1, context);

		int maxlen = avg / context->axis_div;
		if (num < context->triangles_cnt / 10)
			maxlen = avg / context->axis_div * 4;

		if (context->rnd.random_double(0, 1) < 0.3) {
			len1 = context->rnd.random_int(1, maxlen);
			px1 = context->rnd.random_double(-1, 1);
			py1 = context->rnd.random_double(-1, 1);
		}

		if (context->rnd.random_double(0, 1) < 0.3) {
			len2 = context->rnd.random_int(1, maxlen);
			px2 = context->rnd.random_double(-1, 1);
			py2 = context->rnd.random_double(-1, 1);
		}

		if (context->rnd.random_double(0, 1) < 0.3) {
			len3 = context->rnd.random_int(1, maxlen);
			px3 = context->rnd.random_double(-1, 1);
			py3 = context->rnd.random_double(-1, 1);
		}

		recount(w, h);

		evaulate_color(context);
	}

	Json to_json() const {
		return Json::array { x1, y1, x2, y2, x3, y3, r, g, b };
	}
};


class PaintResult: public GenResult<PaintResult, GenPaintContext> {

public:

	vector<PaintTriangle> triangles;

	vector<PaintTriangle>* get_triangles() {
		return &triangles;
	}

	PaintResult(GenPaintContext *context, vector<PaintResult*> &from_population)
		: GenResult(context, from_population) {

		gen_property_vector<PaintTriangle>(&PaintResult::get_triangles);

		evaluate_score();

		parents.clear();
	}

	PaintResult(PaintResult *from) : GenResult(from) {
		triangles = from->triangles;
	}

	Json to_json() {
		return Json::object {
			{ "triangles", Json(triangles) }
		};
	}

	Image* draw() {
		Image *image = context->image;
		Image *res = new Image(image->width(), image->height(), 1, 3);
		res->fill(0);
		for (auto &t: triangles) {
			unsigned char color[] = {(unsigned char)t.r, (unsigned char)t.g, (unsigned char)t.b};
			float opacity = context->opacity;
			res->draw_triangle(t.x1, t.y1, t.x2, t.y2, t.x3, t.y3, color, opacity);
		}
		return res;
	}

private:

	void evaluate_score() {
		Image *image = context->image;
		Image *res = draw();

		score = 0;
		double worst = (255 * 255) * 3;
		for (int i = 0; i < image->width(); ++i)
			for (int j = 0; j < image->height(); ++j) {
				double sum = 0;
				for (int c = 0; c < 3; ++c) {
					int c1 = (*image)(i, j, 0, c);
					int c2 = (*res)(i, j, 0, c);
					sum += (c1 - c2) * (c1 - c2);
				}
				score += worst - sum;
			}

		delete res;
	}

};
