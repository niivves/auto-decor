#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <memory>
#include "Decorator.hpp"
#include "DecorPreview.hpp"

class AutoDecorPopup : public geode::Popup {
public:
	static AutoDecorPopup* create(LevelEditorLayer* lel);

protected:
	bool init(LevelEditorLayer* lel);
	void update(float dt) override;

	void step(int dir);
	void applyStyleVisuals(bool animate);
	void onApply();
	void onUndo();
	void onCapture(PieceSlot slot);
	void setRunning(bool running);
	void drawBar(float progress);

	LevelEditorLayer* m_lel = nullptr;
	size_t m_index = 0;
	bool m_selectedOnly = false;
	bool m_background = true;
	float m_time = 0.f;

	std::unique_ptr<DecoJob> m_job;

	cocos2d::CCLayerGradient* m_panel = nullptr;
	cocos2d::CCLabelBMFont* m_titleLabel = nullptr;
	cocos2d::CCLabelBMFont* m_nameLabel = nullptr;
	cocos2d::CCLabelBMFont* m_descLabel = nullptr;
	cocos2d::CCLabelBMFont* m_countLabel = nullptr;
	cocos2d::CCLayerColor* m_swatches[4] = {};
	DecorPreview* m_preview = nullptr;
	cocos2d::CCDrawNode* m_bar = nullptr;
	cocos2d::CCMenu* m_actionMenu = nullptr;

	// panel tint easing
	cocos2d::ccColor3B m_panelStart{ 10, 10, 30 };
	cocos2d::ccColor3B m_panelEnd{ 30, 10, 60 };
	cocos2d::ccColor3B m_panelStartTarget{ 10, 10, 30 };
	cocos2d::ccColor3B m_panelEndTarget{ 30, 10, 60 };
};
