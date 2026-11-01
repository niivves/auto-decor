#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "AutoDecorPopup.hpp"

using namespace geode::prelude;

class $modify(AutoDecorEditorUI, EditorUI) {
	bool init(LevelEditorLayer* lel) {
		if (!EditorUI::init(lel)) return false;

		auto win = CCDirector::get()->getWinSize();

		auto menu = CCMenu::create();
		menu->setID("decor-menu"_spr);
		menu->setPosition({ win.width * 0.11f, win.height * 0.80f });

		auto spr = ButtonSprite::create("Decor", "bigFont.fnt", "GJ_button_04.png", 0.6f);
		spr->setScale(0.7f);
		auto btn = CCMenuItemExt::createSpriteExtra(spr, [this](auto) {
			if (auto popup = AutoDecorPopup::create(m_editorLayer)) popup->show();
		});
		btn->setID("decor-button"_spr);

		// gentle idle pulse so the button is easy to spot
		btn->runAction(CCRepeatForever::create(CCSequence::create(
			CCEaseSineInOut::create(CCScaleTo::create(0.9f, 1.07f)),
			CCEaseSineInOut::create(CCScaleTo::create(0.9f, 1.f)),
			nullptr
		)));

		menu->addChild(btn);
		this->addChild(menu, 100);
		return true;
	}
};
