#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <vector>

// Which kind of decor piece a list of object IDs is used for.
enum class PieceSlot {
	Overlay = 0,    // sits on top of every existing solid block
	Edge = 1,       // sits on exposed edges of existing blocks (rotated to face outward)
	Hazard = 2,     // sits on top of existing spikes/saws (copies their rotation)
	Background = 3  // sparse pieces behind the layout, in empty space only
};

const char* slotName(PieceSlot slot);

struct DecoStyle {
	std::string name;
	std::string description;

	// Palette, applied through colour triggers so the original objects are
	// recoloured, never replaced.
	cocos2d::ccColor3B background;
	cocos2d::ccColor3B blocks;
	cocos2d::ccColor3B outline;
	cocos2d::ccColor3B accent;
};

const std::vector<DecoStyle>& allStyles();

// Object IDs the user captured from the editor selection for this style + slot.
std::vector<int> piecesFor(const DecoStyle& style, PieceSlot slot);
void savePieces(const DecoStyle& style, PieceSlot slot, std::vector<int> ids);
