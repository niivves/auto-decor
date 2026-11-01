#include "Decorator.hpp"
#include <algorithm>
#include <cmath>
#include <map>

using namespace geode::prelude;

namespace {
	constexpr float CELL = 30.f;

	// Everything the latest job created, so "Undo last" can remove it.
	std::vector<Ref<GameObject>> s_lastBatch;

	std::pair<int, int> cellOf(CCPoint p) {
		return { (int)std::floor(p.x / CELL), (int)std::floor(p.y / CELL) };
	}

	bool isSolid(GameObject* o) { return o->m_objectType == GameObjectType::Solid; }
	bool isHazard(GameObject* o) { return o->m_objectType == GameObjectType::Hazard; }

	// Colour triggers at the start of the level set the palette. These are the
	// dedicated per-channel triggers, so no channel field is needed:
	// 29 = BG, 30 = ground, 105 = objects, 915 = line.
	void placeColourTrigger(LevelEditorLayer* lel, int triggerID, ccColor3B col, float y) {
		auto obj = lel->createObject(triggerID, { 0.f, y }, true);
		if (!obj) return;
		auto trig = static_cast<EffectGameObject*>(obj);
		trig->m_triggerTargetColor = col;
		trig->m_duration = 0.f;
		s_lastBatch.push_back(obj);
	}
}

DecoJob::DecoJob(LevelEditorLayer* lel, const DecoStyle& style, DecoOptions opt)
	: m_lel(lel), m_style(style), m_opt(opt), m_rng(std::random_device{}()) {

	s_lastBatch.clear();

	m_overlay = piecesFor(style, PieceSlot::Overlay);
	m_edge = piecesFor(style, PieceSlot::Edge);
	m_hazard = piecesFor(style, PieceSlot::Hazard);
	m_backdrop = piecesFor(style, PieceSlot::Background);

	// Map every solid block in the whole level so "exposed edge" checks are
	// correct even when only part of the level is selected.
	std::map<int, int> colTop;
	bool first = true;
	for (auto obj : CCArrayExt<GameObject*>(lel->m_objects)) {
		if (!isSolid(obj)) continue;
		auto c = cellOf(obj->getPosition());
		m_solids.insert(c);
		auto it = colTop.find(c.first);
		if (it == colTop.end() || it->second < c.second) colTop[c.first] = c.second;
		if (first || c.second < m_minY) m_minY = c.second;
		first = false;
	}
	for (auto& [x, y] : colTop) m_columnTop.push_back({ x, y });

	if (opt.selectedOnly && lel->m_editorUI) {
		if (auto sel = lel->m_editorUI->getSelectedObjects()) {
			for (auto obj : CCArrayExt<GameObject*>(sel)) m_targets.push_back(obj);
		}
	} else {
		for (auto obj : CCArrayExt<GameObject*>(lel->m_objects)) m_targets.push_back(obj);
	}

	// Palette: recolours the user's existing objects instead of replacing them.
	if (!opt.selectedOnly) {
		placeColourTrigger(lel, 29, style.background, 30.f);
		placeColourTrigger(lel, 30, style.background, 60.f);
		placeColourTrigger(lel, 105, style.blocks, 90.f);
		placeColourTrigger(lel, 915, style.outline, 120.f);
	}

	if (opt.background && !m_backdrop.empty() && !m_columnTop.empty()) {
		int width = (int)m_columnTop.size();
		m_backdropTotal = std::max(1, width * std::clamp(opt.density, 1, 10) / 14);
	}
}

bool DecoJob::hasPieces() const {
	return !(m_overlay.empty() && m_edge.empty() && m_hazard.empty() && m_backdrop.empty());
}

float DecoJob::progress() const {
	float total = (float)(m_targets.size() + m_backdropTotal);
	if (total <= 0.f) return 1.f;
	return std::clamp(((float)m_index + (float)m_backdropDone) / total, 0.f, 1.f);
}

bool DecoJob::chance(float p) {
	return (m_rng() % 10000) < (unsigned)(std::clamp(p, 0.f, 1.f) * 10000.f);
}

int DecoJob::pick(const std::vector<int>& v) {
	return v[m_rng() % v.size()];
}

GameObject* DecoJob::place(int id, CCPoint pos, float rotation, GameObject* copyFrom) {
	auto obj = m_lel->createObject(id, pos, true);
	if (!obj) return nullptr;

	// Safety net: decor must never add hitboxes. If an ID turns out to be a
	// solid or a hazard, throw it away immediately.
	if (isSolid(obj) || isHazard(obj)) {
		m_lel->removeObject(obj, true);
		return nullptr;
	}

	if (copyFrom) {
		obj->setRotation(copyFrom->getRotation());
		obj->setScaleX(copyFrom->getScaleX());
		obj->setScaleY(copyFrom->getScaleY());
	} else if (rotation != 0.f) {
		obj->setRotation(rotation);
	}

	s_lastBatch.push_back(obj);
	m_placed++;
	return obj;
}

void DecoJob::processObject(GameObject* obj) {
	if (!obj) return;
	float p = std::clamp(m_opt.density, 1, 10) / 10.f;
	auto pos = obj->getPosition();

	if (isSolid(obj)) {
		auto c = cellOf(pos);

		// 1. Overlay: decor sitting right on top of the block you placed.
		if (!m_overlay.empty() && chance(p * 0.8f)) {
			place(pick(m_overlay), pos);
		}

		// 2. Edge pieces: only where a side of the block is exposed. They go
		//    on the block's own position (not outside it) and are rotated to
		//    face the open side.
		if (!m_edge.empty()) {
			bool topOpen = !m_solids.count({ c.first, c.second + 1 });
			bool rightOpen = !m_solids.count({ c.first + 1, c.second });
			bool botOpen = !m_solids.count({ c.first, c.second - 1 });
			bool leftOpen = !m_solids.count({ c.first - 1, c.second });
			if (topOpen && chance(p)) place(pick(m_edge), pos, 0.f);
			if (rightOpen && chance(p * 0.7f)) place(pick(m_edge), pos, 90.f);
			if (botOpen && chance(p * 0.5f)) place(pick(m_edge), pos, 180.f);
			if (leftOpen && chance(p * 0.7f)) place(pick(m_edge), pos, 270.f);
		}
	} else if (isHazard(obj)) {
		// 3. Glow behind/over each existing spike or saw, matching its rotation.
		if (!m_hazard.empty() && chance(p)) {
			place(pick(m_hazard), pos, 0.f, obj);
		}
	}
}

void DecoJob::placeBackdrop() {
	// Sparse pieces in EMPTY space just above the layout. Never inside blocks.
	for (int attempt = 0; attempt < 6; attempt++) {
		auto [cx, top] = m_columnTop[m_rng() % m_columnTop.size()];
		int cy = top + 2 + (int)(m_rng() % 5);
		if (m_solids.count({ cx, cy })) continue;
		place(pick(m_backdrop), { cx * CELL + CELL / 2.f, cy * CELL + CELL / 2.f });
		return;
	}
}

bool DecoJob::step(int budget) {
	int n = 0;
	while (m_index < m_targets.size() && n < budget) {
		processObject(m_targets[m_index++]);
		n++;
	}
	while (m_index >= m_targets.size() && m_backdropDone < m_backdropTotal && n < budget) {
		placeBackdrop();
		m_backdropDone++;
		n++;
	}
	return m_index >= m_targets.size() && m_backdropDone >= m_backdropTotal;
}

int removeLastDeco(LevelEditorLayer* lel) {
	int removed = 0;
	for (auto& obj : s_lastBatch) {
		if (obj && lel->m_objects->containsObject(obj)) {
			lel->removeObject(obj, true);
			removed++;
		}
	}
	s_lastBatch.clear();
	return removed;
}
