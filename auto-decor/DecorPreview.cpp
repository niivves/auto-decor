#include "DecorPreview.hpp"
#include <cmath>

using namespace geode::prelude;

namespace {
	ccColor4F f4(ccColor3B c, float a = 1.f) {
		return { c.r / 255.f, c.g / 255.f, c.b / 255.f, a };
	}
	ccColor4F withA(ccColor4F c, float a) { return { c.r, c.g, c.b, a }; }
	ccColor4F darker(ccColor4F c, float k) { return { c.r * k, c.g * k, c.b * k, c.a }; }

	std::vector<CCPoint> rectPts(float x, float y, float w, float h) {
		return { { x, y }, { x + w, y }, { x + w, y + h }, { x, y + h } };
	}
	std::vector<CCPoint> triPts(float x, float y, float w, float h) {
		return { { x, y }, { x + w, y }, { x + w / 2.f, y + h } };
	}
	std::vector<CCPoint> circlePts(float cx, float cy, float r) {
		std::vector<CCPoint> v;
		for (int i = 0; i < 28; i++) {
			float a = (float)i / 28.f * 6.2831853f;
			v.push_back({ cx + std::cos(a) * r, cy + std::sin(a) * r });
		}
		return v;
	}

	// Sutherland-Hodgman against a vertical line: keep the left or right side.
	std::vector<CCPoint> clipX(const std::vector<CCPoint>& poly, float cut, bool keepLeft) {
		std::vector<CCPoint> out;
		size_t n = poly.size();
		for (size_t i = 0; i < n; i++) {
			CCPoint a = poly[i], b = poly[(i + 1) % n];
			bool ain = keepLeft ? a.x <= cut : a.x >= cut;
			bool bin = keepLeft ? b.x <= cut : b.x >= cut;
			if (ain) out.push_back(a);
			if (ain != bin) {
				float t = (cut - a.x) / (b.x - a.x);
				out.push_back({ cut, a.y + t * (b.y - a.y) });
			}
		}
		return out;
	}

	enum Kind { Ground, Block, Spike };
	struct Shape { std::vector<CCPoint> pts; Kind kind; float x, y, w, h; };

	const std::vector<Shape>& scene() {
		static const std::vector<Shape> s = {
			{ rectPts(0, 0, 230, 16), Ground, 0, 0, 230, 16 },
			{ rectPts(18, 16, 52, 34), Block, 18, 16, 52, 34 },
			{ rectPts(98, 16, 34, 58), Block, 98, 16, 34, 58 },
			{ rectPts(160, 16, 54, 26), Block, 160, 16, 54, 26 },
			{ triPts(24, 50, 12, 14), Spike, 0, 0, 0, 0 },
			{ triPts(40, 50, 12, 14), Spike, 0, 0, 0, 0 },
			{ triPts(102, 74, 12, 14), Spike, 0, 0, 0, 0 },
			{ triPts(76, 16, 12, 14), Spike, 0, 0, 0, 0 },
			{ triPts(138, 16, 12, 14), Spike, 0, 0, 0, 0 },
			{ triPts(176, 42, 12, 14), Spike, 0, 0, 0, 0 },
		};
		return s;
	}
}

DecorPreview* DecorPreview::create(const DecoStyle& style) {
	auto ret = new DecorPreview();
	if (ret->init(style)) {
		ret->autorelease();
		return ret;
	}
	delete ret;
	return nullptr;
}

float DecorPreview::rnd(float a, float b) {
	return a + (b - a) * ((float)(m_rng() % 10000) / 10000.f);
}

bool DecorPreview::init(const DecoStyle& style) {
	if (!CCNode::init()) return false;
	m_rng.seed(std::random_device{}());

	this->setContentSize({ W, H });
	this->setAnchorPoint({ 0.5f, 0.5f });
	this->ignoreAnchorPointForPosition(false);

	m_draw = CCDrawNode::create();
	this->addChild(m_draw);

	auto before = CCLabelBMFont::create("BEFORE", "bigFont.fnt");
	before->setScale(0.28f);
	before->setOpacity(190);
	before->setPosition({ 28.f, H - 10.f });
	this->addChild(before, 2);

	auto after = CCLabelBMFont::create("AFTER", "bigFont.fnt");
	after->setScale(0.28f);
	after->setOpacity(190);
	after->setPosition({ W - 26.f, H - 10.f });
	this->addChild(after, 2);

	m_target[0] = f4(style.background);
	m_target[1] = f4(style.blocks);
	m_target[2] = f4(style.outline);
	m_target[3] = f4(style.accent);
	for (int i = 0; i < 4; i++) m_cur[i] = m_target[i];

	m_sparkles.resize(16);
	for (auto& s : m_sparkles) respawn(s, true);

	this->scheduleUpdate();
	this->redraw();
	return true;
}

void DecorPreview::respawn(Sparkle& s, bool anywhere) {
	s.pos = CCPoint(rnd(4.f, W - 4.f), anywhere ? rnd(4.f, H - 4.f) : rnd(2.f, 18.f));
	s.vel = CCPoint(rnd(-4.f, 4.f), rnd(6.f, 18.f));
	s.maxLife = rnd(1.4f, 3.2f);
	s.life = anywhere ? rnd(0.f, s.maxLife) : s.maxLife;
	s.size = rnd(0.8f, 1.9f);
	s.fromBurst = false;
}

void DecorPreview::setStyle(const DecoStyle& style) {
	m_target[0] = f4(style.background);
	m_target[1] = f4(style.blocks);
	m_target[2] = f4(style.outline);
	m_target[3] = f4(style.accent);

	this->stopActionByTag(77);
	this->setScale(0.93f);
	auto pop = CCEaseBackOut::create(CCScaleTo::create(0.4f, 1.f));
	pop->setTag(77);
	this->runAction(pop);
}

void DecorPreview::burst() {
	for (int i = 0; i < 44; i++) {
		Sparkle s;
		float a = rnd(0.f, 6.2831853f);
		float sp = rnd(30.f, 110.f);
		s.pos = CCPoint(W / 2.f, H / 2.f);
		s.vel = CCPoint(std::cos(a) * sp, std::sin(a) * sp);
		s.maxLife = rnd(0.6f, 1.3f);
		s.life = s.maxLife;
		s.size = rnd(1.2f, 2.6f);
		s.fromBurst = true;
		m_sparkles.push_back(s);
	}
}

void DecorPreview::update(float dt) {
	dt = std::min(dt, 0.05f);
	m_time += dt;

	// ease palette toward the selected style
	float k = std::min(1.f, dt * 7.f);
	for (int i = 0; i < 4; i++) {
		m_cur[i].r += (m_target[i].r - m_cur[i].r) * k;
		m_cur[i].g += (m_target[i].g - m_cur[i].g) * k;
		m_cur[i].b += (m_target[i].b - m_cur[i].b) * k;
	}

	for (auto& s : m_sparkles) {
		s.life -= dt;
		s.pos.x += s.vel.x * dt;
		s.pos.y += s.vel.y * dt;
		if (s.fromBurst) {
			s.vel.x *= 0.96f;
			s.vel.y = s.vel.y * 0.96f - 40.f * dt;
		} else if (s.life <= 0.f || s.pos.y > H) {
			respawn(s, false);
		}
	}
	// drop finished burst sparkles
	std::erase_if(m_sparkles, [](const Sparkle& s) { return s.fromBurst && s.life <= 0.f; });

	this->redraw();
}

void DecorPreview::redraw() {
	m_draw->clear();

	// the wipe line sweeps back and forth
	float wipe = 0.5f + 0.5f * std::sin(m_time * 0.9f);
	float cut = 10.f + wipe * (W - 20.f);

	auto drawPoly = [&](const std::vector<CCPoint>& pts, ccColor4F fill, float bw, ccColor4F border, bool keepLeft) {
		auto clipped = clipX(pts, cut, keepLeft);
		if (clipped.size() < 3) return;
		m_draw->drawPolygon(clipped.data(), (unsigned)clipped.size(), fill, bw, border);
	};
	auto drawSeg = [&](float x0, float x1, float y, ccColor4F col, float r, bool keepLeft) {
		if (keepLeft) x1 = std::min(x1, cut); else x0 = std::max(x0, cut);
		if (x1 <= x0) return;
		m_draw->drawSegment({ x0, y }, { x1, y }, r, col);
	};

	// ---------- BEFORE (left of the wipe): plain layout, white outlines ----------
	ccColor4F bBg = { 0.04f, 0.24f, 0.80f, 1.f };
	ccColor4F white = { 1.f, 1.f, 1.f, 1.f };
	drawPoly(rectPts(0, 0, W, H), bBg, 0, bBg, true);
	for (auto& sh : scene()) {
		if (sh.kind == Ground) drawPoly(sh.pts, { 0.05f, 0.30f, 0.92f, 1.f }, 1.2f, white, true);
		else if (sh.kind == Block) drawPoly(sh.pts, withA(bBg, 1.f), 1.2f, white, true);
		else drawPoly(sh.pts, { 0.f, 0.f, 0.f, 1.f }, 1.2f, white, true);
	}

	// ---------- AFTER (right of the wipe): same layout, styled ----------
	ccColor4F bg = m_cur[0], blk = m_cur[1], out = m_cur[2], acc = m_cur[3];
	drawPoly(rectPts(0, 0, W, H), bg, 0, bg, false);

	// backdrop piece (a soft planet) - the kind of thing the Backdrop slot adds
	drawPoly(circlePts(150.f, 84.f, 21.f), withA(acc, 0.16f), 0, withA(acc, 0.f), false);
	drawPoly(circlePts(150.f, 84.f, 21.f), withA(acc, 0.f), 1.2f, withA(acc, 0.28f), false);

	for (auto& sh : scene()) {
		if (sh.kind == Ground) {
			drawPoly(sh.pts, blk, 1.6f, out, false);
		} else if (sh.kind == Block) {
			drawPoly(sh.pts, withA(blk, 0.f), 4.5f, withA(out, 0.20f), false); // glow
			drawPoly(sh.pts, blk, 1.6f, out, false);
			drawSeg(sh.x + 5, sh.x + sh.w - 5, sh.y + sh.h * 0.36f, withA(acc, 0.75f), 0.8f, false);
			drawSeg(sh.x + 5, sh.x + sh.w - 5, sh.y + sh.h * 0.66f, withA(acc, 0.45f), 0.8f, false);
		} else {
			drawPoly(sh.pts, withA(blk, 0.f), 4.5f, withA(out, 0.22f), false);
			drawPoly(sh.pts, darker(blk, 0.7f), 1.6f, out, false);
		}
	}

	// ---------- wipe line ----------
	float pulse = 0.55f + 0.25f * std::sin(m_time * 5.f);
	m_draw->drawSegment({ cut, 2.f }, { cut, H - 2.f }, 4.f, withA(out, 0.18f));
	m_draw->drawSegment({ cut, 2.f }, { cut, H - 2.f }, 1.1f, { 1.f, 1.f, 1.f, 0.95f });
	m_draw->drawDot({ cut, H / 2.f }, 5.5f, withA(out, pulse));
	m_draw->drawDot({ cut, H / 2.f }, 3.f, { 1.f, 1.f, 1.f, 1.f });

	// ---------- sparkles ----------
	for (auto& s : m_sparkles) {
		float t = std::clamp(s.life / s.maxLife, 0.f, 1.f);
		float a = s.fromBurst ? t : std::sin(t * 3.14159f);
		auto col = s.fromBurst ? out : acc;
		m_draw->drawDot(s.pos, s.size * (0.6f + 0.4f * t), withA(col, 0.85f * a));
	}

	// ---------- frame ----------
	float fa = 0.55f + 0.25f * std::sin(m_time * 2.f);
	auto frame = rectPts(0.5f, 0.5f, W - 1.f, H - 1.f);
	m_draw->drawPolygon(frame.data(), 4, { 0, 0, 0, 0 }, 1.5f, withA(out, fa));
}
