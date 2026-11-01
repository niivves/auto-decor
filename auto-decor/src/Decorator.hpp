#pragma once
#include <Geode/Geode.hpp>
#include <random>
#include <set>
#include <utility>
#include <vector>
#include "Styles.hpp"

struct DecoOptions {
	int density = 6;            // 1..10
	bool selectedOnly = false;  // only decorate the current editor selection
	bool background = true;     // allow backdrop pieces in empty space
};

// Adds decor to a layout that already exists. It never moves, deletes or
// replaces the user's objects and never builds new solid structures.
// Work is split into steps so the UI can animate progress.
class DecoJob {
public:
	DecoJob(LevelEditorLayer* lel, const DecoStyle& style, DecoOptions opt);

	// Handle up to `budget` objects. Returns true when finished.
	bool step(int budget);

	float progress() const;
	int placed() const { return m_placed; }
	bool hasPieces() const;

private:
	using Cell = std::pair<int, int>;

	void processObject(GameObject* obj);
	void placeBackdrop();
	GameObject* place(int id, cocos2d::CCPoint pos, float rotation = 0.f, GameObject* copyFrom = nullptr);
	bool chance(float p);
	int pick(const std::vector<int>& v);

	LevelEditorLayer* m_lel;
	DecoStyle m_style;
	DecoOptions m_opt;
	std::mt19937 m_rng;

	std::vector<int> m_overlay, m_edge, m_hazard, m_backdrop;

	std::set<Cell> m_solids;
	std::vector<geode::Ref<GameObject>> m_targets;
	std::set<int> m_columns;
	std::vector<std::pair<int, int>> m_columnTop; // (cellX, top cellY)
	int m_minY = 0;

	size_t m_index = 0;
	int m_backdropTotal = 0;
	int m_backdropDone = 0;
	int m_placed = 0;
};

// Removes everything the most recent job added (that is still in the level).
int removeLastDeco(LevelEditorLayer* lel);
