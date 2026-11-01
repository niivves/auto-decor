#pragma once
#include <Geode/Geode.hpp>
#include <random>
#include <vector>
#include "Styles.hpp"

// Animated mini scene: a wipe line sweeps back and forth, showing the same
// little layout "before" (plain outlines) and "after" (styled). Colours ease
// toward the selected style, and sparkles drift around.
class DecorPreview : public cocos2d::CCNode {
public:
	static constexpr float W = 230.f;
	static constexpr float H = 112.f;

	static DecorPreview* create(const DecoStyle& style);

	void setStyle(const DecoStyle& style); // eases colours + a little pop
	void burst();                          // celebration sparkles

protected:
	bool init(const DecoStyle& style);
	void update(float dt) override;
	void redraw();

	struct Sparkle {
		cocos2d::CCPoint pos;
		cocos2d::CCPoint vel;
		float life = 0.f;
		float maxLife = 1.f;
		float size = 1.f;
		bool fromBurst = false;
	};

	void respawn(Sparkle& s, bool anywhere);
	float rnd(float a, float b);

	cocos2d::CCDrawNode* m_draw = nullptr;
	cocos2d::ccColor4F m_cur[4];    // background, blocks, outline, accent
	cocos2d::ccColor4F m_target[4];
	std::vector<Sparkle> m_sparkles;
	std::mt19937 m_rng;
	float m_time = 0.f;
};
