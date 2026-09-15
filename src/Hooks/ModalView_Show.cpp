#include "hooking.hpp"

#include "HMUI/ModalView.hpp"
#include "HMUI/Screen.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/GameObject.hpp"

MAKE_AUTO_HOOK_MATCH(ModalView_Show, &HMUI::ModalView::Show, void, HMUI::ModalView* self, bool animated, bool moveToCenter, System::Action* finishedCallback)
{
	ModalView_Show(self, animated, moveToCenter, finishedCallback);
	auto cb = self->_blockerGO->get_gameObject()->GetComponent<UnityEngine::Canvas*>();
	auto parent = self->get_transform()->get_parent();
	auto screen = parent ? parent->GetComponentInParent<HMUI::Screen*>() : nullptr;
	int highest = 1;
	if (screen) {
		// Match PC: inactive canvases must not influence modal stacking.
		auto canvases = screen->GetComponentsInChildren<UnityEngine::Canvas*>();
		bool foundCanvas = false;
		int maxOrder = 0;
		for (auto canvas : canvases) {
			if (canvas->get_sortingLayerID() == cb->get_sortingLayerID()) {
				if (!foundCanvas || canvas->get_sortingOrder() > maxOrder) {
					maxOrder = canvas->get_sortingOrder();
				}
				foundCanvas = true;
			}
		}
		highest = maxOrder + 1;
	}
	cb->set_overrideSorting(true);
	cb->set_sortingOrder(highest);

	auto cm = self->get_gameObject()->GetComponent<UnityEngine::Canvas*>();
	cm->set_overrideSorting(true);
	cm->set_sortingOrder(highest + 1);
}
