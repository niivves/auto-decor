#include "AutoDecorPopup.hpp"
#include <algorithm>
#include <cmath>
#include <set>

using namespace geode::prelude;

namespace {
	constexpr float POPUP_W = 380.f;
	constexpr float POPUP_H = 320.f;
	constexpr float TITLE_SCALE = 0.78f;

	GLubyte lerpByte(GLubyte a, GLubyte b, float t) {
		return (GLubyte)std::clamp(a + (b - a) * t, 0.f, 255.f);
	}
	ccColor3B lerpCol(ccColor3B a, ccColor3B b, float t) {
		return { lerpByte(a.r, b.r, t), lerpByte(a.g, b.g, t), lerpByte(a.b, b.b, t) };
	}
	ccColor3B scaled(ccColor3B c, float k) {
		return { (GLubyte)std::clamp(c.r * k, 0.f, 255.f), (GLubyte)std::clamp(c.g * k, 0.f, 255.f), (GLubyte)std::clamp(c.b * k, 0.f, 255.f) };
	}

	// scale-in with a little overshoot, staggered by index
	void popIn(CCNode* node, int index) {
		float s = node->getScale();
		node->setScale(0.f);
		node->runAction(CCSequence::create(
			CCDelayTime::create(0.10f + 0.05f * index),
			CCEaseBackOut::create(CCScaleTo::create(0.4f, s)),
			nullptr
		));
	}

	void breathe(CCNode* node, float amount, float seconds) {
		float s = node->getScale();
		node->runAction(CCRepeatForever::create(CCSequence::create(
			CCEaseSineInOut::create(CCScaleTo::create(seconds, s * (1.f + amount))),
			CCEaseSineInOut::create(CCScaleTo::create(seconds, s)),
			nullptr
		)));
	}
}

AutoDecorPopup* AutoDecorPopup::create(LevelEditorLayer* lel) {
	auto ret = new AutoDecorPopup();
	if (ret->init(lel)) {
		ret->autorelease();
		return ret;
	}
	delete ret;
	return nullptr;
}

bool AutoDecorPopup::init(LevelEditorLayer* lel) {
	if (!Popup::init(POPUP_W, POPUP_H)) return false;
	m_lel = lel;

	auto size = m_mainLayer->getContentSize();
	float cx = size.width / 2.f;
	float H = size.height;
	int order = 0;

	// ---- tinted gradient panel (eases toward the selected style) ----
	m_panel = CCLayerGradient::create({ 10, 10, 30, 255 }, { 30, 10, 60, 255 });
	m_panel->setContentSize({ size.width - 20.f, size.height - 20.f });
	m_panel->setPosition({ 10.f, 10.f });
	m_mainLayer->addChild(m_panel);

	// ---- title ----
	m_titleLabel = CCLabelBMFont::create("Auto Decor", "bigFont.fnt");
	m_titleLabel->setScale(TITLE_SCALE);
	m_titleLabel->setPosition({ cx, H - 26.f });
	m_mainLayer->addChild(m_titleLabel);
	popIn(m_titleLabel, order++);
	m_titleLabel->runAction(CCRepeatForever::create(CCSequence::create(
		CCTintTo::create(1.6f, 255, 215, 120),
		CCTintTo::create(1.6f, 255, 255, 255),
		nullptr
	)));

	auto byLabel = CCLabelBMFont::create("by Nives", "goldFont.fnt");
	byLabel->setScale(0.5f);
	byLabel->setPosition({ cx, H - 48.f });
	m_mainLayer->addChild(byLabel);
	popIn(byLabel, order++);
	breathe(byLabel, 0.05f, 1.4f);

	// ---- animated before/after preview ----
	m_preview = DecorPreview::create(allStyles()[0]);
	m_preview->setPosition({ cx, H - 112.f });
	m_mainLayer->addChild(m_preview);
	popIn(m_preview, order++);

	// ---- style labels ----
	m_nameLabel = CCLabelBMFont::create("", "goldFont.fnt");
	m_nameLabel->setScale(0.75f);
	m_nameLabel->setPosition({ cx, H - 184.f });
	m_mainLayer->addChild(m_nameLabel);

	m_descLabel = CCLabelBMFont::create("", "chatFont.fnt");
	m_descLabel->setScale(0.62f);
	m_descLabel->setPosition({ cx, H - 202.f });
	m_mainLayer->addChild(m_descLabel);

	m_countLabel = CCLabelBMFont::create("", "chatFont.fnt");
	m_countLabel->setScale(0.5f);
	m_countLabel->setOpacity(150);
	m_countLabel->setPosition({ cx, H - 215.f });
	m_mainLayer->addChild(m_countLabel);

	// ---- palette swatches ----
	for (int i = 0; i < 4; i++) {
		auto sw = CCLayerColor::create({ 255, 255, 255, 255 }, 34.f, 14.f);
		sw->ignoreAnchorPointForPosition(false);
		sw->setAnchorPoint({ 0.5f, 0.5f });
		sw->setPosition({ cx - 57.f + i * 38.f, H - 232.f });
		m_mainLayer->addChild(sw);
		m_swatches[i] = sw;
		popIn(sw, order++);
	}

	// ---- style arrows ----
	auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
	auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
	rightSpr->setFlipX(true);
	auto leftBtn = CCMenuItemExt::createSpriteExtra(leftSpr, [this](auto) { this->step(-1); });
	auto rightBtn = CCMenuItemExt::createSpriteExtra(rightSpr, [this](auto) { this->step(1); });
	leftBtn->setPosition({ 34.f, H - 112.f });
	rightBtn->setPosition({ size.width - 34.f, H - 112.f });
	m_buttonMenu->addChild(leftBtn);
	m_buttonMenu->addChild(rightBtn);
	popIn(leftBtn, order++);
	popIn(rightBtn, order++);

	// ---- capture decor pieces from the editor selection ----
	auto capLabel = CCLabelBMFont::create("Set pieces from selected deco:", "chatFont.fnt");
	capLabel->setScale(0.52f);
	capLabel->setOpacity(190);
	capLabel->setPosition({ cx, H - 250.f });
	m_mainLayer->addChild(capLabel);

	const PieceSlot slots[4] = { PieceSlot::Overlay, PieceSlot::Edge, PieceSlot::Hazard, PieceSlot::Background };
	for (int i = 0; i < 4; i++) {
		auto slot = slots[i];
		auto spr = ButtonSprite::create(slotName(slot), "bigFont.fnt", "GJ_button_05.png", 0.7f);
		spr->setScale(0.5f);
		auto btn = CCMenuItemExt::createSpriteExtra(spr, [this, slot](auto) { this->onCapture(slot); });
		btn->setPosition({ cx - 117.f + i * 78.f, H - 266.f });
		m_buttonMenu->addChild(btn);
		popIn(btn, order++);
	}

	// ---- options ----
	auto selToggle = CCMenuItemExt::createTogglerWithStandardSprites(0.5f, [this](auto t) {
		m_selectedOnly = !t->isToggled(); // callback sees the state before the flip
	});
	selToggle->setPosition({ 52.f, H - 288.f });
	m_buttonMenu->addChild(selToggle);
	auto selLabel = CCLabelBMFont::create("Selection only", "bigFont.fnt");
	selLabel->setScale(0.3f);
	selLabel->setAnchorPoint({ 0.f, 0.5f });
	selLabel->setPosition({ 68.f, H - 288.f });
	m_mainLayer->addChild(selLabel);

	auto bgToggle = CCMenuItemExt::createTogglerWithStandardSprites(0.5f, [this](auto t) {
		m_background = !t->isToggled();
	});
	bgToggle->toggle(true);
	bgToggle->setPosition({ cx + 18.f, H - 288.f });
	m_buttonMenu->addChild(bgToggle);
	auto bgLabel = CCLabelBMFont::create("Backdrop pieces", "bigFont.fnt");
	bgLabel->setScale(0.3f);
	bgLabel->setAnchorPoint({ 0.f, 0.5f });
	bgLabel->setPosition({ cx + 34.f, H - 288.f });
	m_mainLayer->addChild(bgLabel);

	// ---- action buttons (swapped for a progress bar while running) ----
	m_actionMenu = CCMenu::create();
	m_actionMenu->setPosition({ 0.f, 0.f });
	m_actionMenu->setContentSize(size);
	m_mainLayer->addChild(m_actionMenu);

	auto undoBtn = CCMenuItemExt::createSpriteExtra(
		ButtonSprite::create("Undo last", "bigFont.fnt", "GJ_button_06.png", 0.7f),
		[this](auto) { this->onUndo(); }
	);
	undoBtn->setPosition({ cx - 75.f, 26.f });
	m_actionMenu->addChild(undoBtn);
	popIn(undoBtn, order++);

	auto applyBtn = CCMenuItemExt::createSpriteExtra(
		ButtonSprite::create("Decorate!"),
		[this](auto) { this->onApply(); }
	);
	applyBtn->setPosition({ cx + 70.f, 26.f });
	m_actionMenu->addChild(applyBtn);
	popIn(applyBtn, order++);
	breathe(applyBtn, 0.045f, 0.7f);

	// ---- progress bar ----
	m_bar = CCDrawNode::create();
	m_bar->setPosition({ cx, 26.f });
	m_bar->setVisible(false);
	m_mainLayer->addChild(m_bar);

	this->applyStyleVisuals(false);
	this->scheduleUpdate();
	return true;
}

void AutoDecorPopup::step(int dir) {
	auto n = allStyles().size();
	m_index = (m_index + n + dir) % n;
	this->applyStyleVisuals(true);
}

void AutoDecorPopup::applyStyleVisuals(bool animate) {
	auto& s = allStyles()[m_index];

	m_nameLabel->setString(s.name.c_str());
	m_descLabel->setString(s.description.c_str());
	m_countLabel->setString(fmt::format("{} / {}", m_index + 1, allStyles().size()).c_str());

	ccColor3B cols[4] = { s.background, s.blocks, s.outline, s.accent };
	for (int i = 0; i < 4; i++) {
		if (animate) {
			m_swatches[i]->stopAllActions();
			m_swatches[i]->setScale(0.7f);
			m_swatches[i]->runAction(CCSequence::create(
				CCDelayTime::create(0.04f * i),
				CCSpawn::create(
					CCTintTo::create(0.3f, cols[i].r, cols[i].g, cols[i].b),
					CCEaseBackOut::create(CCScaleTo::create(0.35f, 1.f)),
					nullptr
				),
				nullptr
			));
		} else {
			m_swatches[i]->setColor(cols[i]);
		}
	}

	m_panelStartTarget = scaled(s.background, 0.55f);
	m_panelEndTarget = scaled(s.blocks, 0.45f);

	if (animate) {
		m_preview->setStyle(s);

		m_nameLabel->stopAllActions();
		m_nameLabel->setScale(0.5f);
		m_nameLabel->runAction(CCEaseBackOut::create(CCScaleTo::create(0.35f, 0.75f)));

		m_descLabel->stopAllActions();
		m_descLabel->setOpacity(0);
		m_descLabel->runAction(CCFadeIn::create(0.3f));
	} else {
		m_preview->setStyle(s);
		m_panelStart = m_panelStartTarget;
		m_panelEnd = m_panelEndTarget;
		m_panel->setStartColor(m_panelStart);
		m_panel->setEndColor(m_panelEnd);
	}
}

void AutoDecorPopup::setRunning(bool running) {
	// a hidden menu ignores touches, so no separate enabled flag is needed
	m_actionMenu->setVisible(!running);
	m_bar->setVisible(running);
	if (running) this->drawBar(0.f);
}

void AutoDecorPopup::drawBar(float p) {
	auto& s = allStyles()[m_index];
	float w = 260.f;
	float x0 = -w / 2.f;
	auto out = ccColor4F{ s.outline.r / 255.f, s.outline.g / 255.f, s.outline.b / 255.f, 1.f };
	auto acc = ccColor4F{ s.accent.r / 255.f, s.accent.g / 255.f, s.accent.b / 255.f, 1.f };

	m_bar->clear();
	m_bar->drawSegment({ x0, 0.f }, { x0 + w, 0.f }, 6.f, { 1.f, 1.f, 1.f, 0.12f });
	if (p > 0.001f) {
		m_bar->drawSegment({ x0, 0.f }, { x0 + w * p, 0.f }, 6.f, out);
		// shimmer travelling across the filled part
		float sx = x0 + std::fmod(m_time * 240.f, std::max(1.f, w * p));
		m_bar->drawDot({ sx, 0.f }, 4.5f, { acc.r, acc.g, acc.b, 0.85f });
	}
}

void AutoDecorPopup::update(float dt) {
	dt = std::min(dt, 0.05f);
	m_time += dt;

	// ease the panel tint
	float k = std::min(1.f, dt * 6.f);
	m_panelStart = lerpCol(m_panelStart, m_panelStartTarget, k);
	m_panelEnd = lerpCol(m_panelEnd, m_panelEndTarget, k);
	m_panel->setStartColor(m_panelStart);
	m_panel->setEndColor(m_panelEnd);

	// advance the decorating job a few objects per frame so the bar animates
	if (m_job) {
		bool done = m_job->step(120);
		this->drawBar(m_job->progress());
		if (done) {
			int placed = m_job->placed();
			m_job.reset();
			this->setRunning(false);
			m_preview->burst();
			Notification::create(
				fmt::format("Done! Added {} decor objects.", placed),
				NotificationIcon::Success
			)->show();
		}
	}
}

void AutoDecorPopup::onApply() {
	if (m_job) return;

	auto style = allStyles()[m_index];
	DecoOptions opt;
	opt.density = (int)Mod::get()->getSettingValue<int64_t>("density");
	opt.selectedOnly = m_selectedOnly;
	opt.background = m_background;

	m_job = std::make_unique<DecoJob>(m_lel, style, opt);

	if (!m_job->hasPieces()) {
		Notification::create(
			"No decor pieces set for this style yet: only the colours will be applied. Select some deco in the editor and use the buttons above.",
			NotificationIcon::Warning, 4.f
		)->show();
	}
	this->setRunning(true);
}

void AutoDecorPopup::onUndo() {
	if (m_job) return;
	int n = removeLastDeco(m_lel);
	Notification::create(
		n > 0 ? fmt::format("Removed {} objects.", n) : std::string("Nothing to undo."),
		n > 0 ? NotificationIcon::Success : NotificationIcon::Info
	)->show();
}

void AutoDecorPopup::onCapture(PieceSlot slot) {
	auto& style = allStyles()[m_index];

	std::vector<int> ids;
	std::set<int> seen;
	int skipped = 0;
	if (m_lel->m_editorUI) {
		if (auto sel = m_lel->m_editorUI->getSelectedObjects()) {
			for (auto obj : CCArrayExt<GameObject*>(sel)) {
				// decor must not have hitboxes
				if (obj->m_objectType == GameObjectType::Solid || obj->m_objectType == GameObjectType::Hazard) {
					skipped++;
					continue;
				}
				if (seen.insert(obj->m_objectID).second) ids.push_back(obj->m_objectID);
			}
		}
	}

	if (ids.empty()) {
		Notification::create(
			skipped > 0 ? "Those are solid blocks or hazards. Select decoration objects instead."
			            : "Select some decoration objects in the editor first.",
			NotificationIcon::Warning
		)->show();
		return;
	}

	savePieces(style, slot, ids);
	Notification::create(
		fmt::format("{}: saved {} piece type(s) for {}.", slotName(slot), ids.size(), style.name),
		NotificationIcon::Success
	)->show();
}
