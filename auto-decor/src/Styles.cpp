#include "Styles.hpp"

using namespace geode::prelude;

static ccColor3B hex(int rgb) {
	return { (GLubyte)((rgb >> 16) & 0xFF), (GLubyte)((rgb >> 8) & 0xFF), (GLubyte)(rgb & 0xFF) };
}

const char* slotName(PieceSlot slot) {
	switch (slot) {
		case PieceSlot::Overlay: return "Overlay";
		case PieceSlot::Edge: return "Edge";
		case PieceSlot::Hazard: return "Glow";
		case PieceSlot::Background: return "Backdrop";
	}
	return "?";
}

const std::vector<DecoStyle>& allStyles() {
	static const std::vector<DecoStyle> styles = {
		{ "Lagoon",    "A flat blue sea: soft glow, tidy outlines.",
		  hex(0x053EB8), hex(0x061E64), hex(0x64EDFF), hex(0x8EC0DF) },
		{ "Neon",      "Hot pink and violet, bright edges, dark base.",
		  hex(0x2A0A4A), hex(0x1A0630), hex(0xFF5CF4), hex(0xB46BFF) },
		{ "Nightmare", "Black and red, sharp edges, heavy shadows.",
		  hex(0x120404), hex(0x1E0505), hex(0xFF2A2A), hex(0x8A1010) },
		{ "Mint",      "Calm green. Minimal and clean.",
		  hex(0x0E4A3C), hex(0x083228), hex(0x7CFFD4), hex(0x4DD6A8) },
		{ "Sunset",    "Warm orange and magenta over deep purple.",
		  hex(0x3A1048), hex(0x240A30), hex(0xFFA34D), hex(0xFF5C8A) },
		{ "Glacier",   "Icy white-blue, crisp and bright.",
		  hex(0x16324F), hex(0x0E2036), hex(0xD8F4FF), hex(0x7FC8F0) },
	};
	return styles;
}

static std::string keyFor(const DecoStyle& style, PieceSlot slot) {
	return fmt::format("pieces-{}-{}", style.name, (int)slot);
}

std::vector<int> piecesFor(const DecoStyle& style, PieceSlot slot) {
	return Mod::get()->getSavedValue<std::vector<int>>(keyFor(style, slot), {});
}

void savePieces(const DecoStyle& style, PieceSlot slot, std::vector<int> ids) {
	Mod::get()->setSavedValue(keyFor(style, slot), std::move(ids));
}
