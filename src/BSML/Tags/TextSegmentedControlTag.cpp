#include "BSML/Tags/TextSegmentedControlTag.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "logging.hpp"
#include "Helpers/getters.hpp"

#include "BSML/Components/TabSelector.hpp"
#include "HMUI/TextSegmentedControl.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"

#include "beatsaber-hook/shared/safeptr.hpp"

using namespace UnityEngine;

namespace BSML {
    static BSMLNodeParser<TextSegmentedControlTag> textSegmentedControlTag({"text-segments"});

    HMUI::TextSegmentedControl* get_textSegmentedControlTemplate() {
        static safe_ptr<HMUI::TextSegmentedControl*> textSegmentedControlTemplate;
        if (!textSegmentedControlTemplate) {
            textSegmentedControlTemplate = Helpers::GetDiContainer()->Resolve<GlobalNamespace::StandardLevelDetailViewController*>()->_standardLevelDetailView->_beatmapDifficultySegmentedControlController->GetComponent<HMUI::TextSegmentedControl*>();
        }
        return textSegmentedControlTemplate.ptr();
    }

    UnityEngine::GameObject* TextSegmentedControlTag::CreateObject(UnityEngine::Transform* parent) const {
        DEBUG("Creating TextSegmentedControl");

        auto textSegmentedControlTemplate = get_textSegmentedControlTemplate();
        auto gameObject = Helpers::GetDiContainer()->InstantiatePrefab(textSegmentedControlTemplate, parent);
        auto textSegmentedControl = gameObject->GetComponent<HMUI::TextSegmentedControl*>();
        textSegmentedControl->_dataSource = nullptr;

        gameObject->set_name("BSMLTextSegmentedControl");

        auto transform = gameObject->transform.cast<RectTransform>();
        transform->set_anchoredPosition({0, 0});
        int childCount = transform->get_childCount();
        for (int i = 1; i <= childCount; i++) {
            Object::DestroyImmediate(transform->GetChild(childCount - i)->get_gameObject());
        }

        Object::Destroy(gameObject->GetComponent<GlobalNamespace::BeatmapDifficultySegmentedControlController*>());
        gameObject->SetActive(true);
        return gameObject;
    }
}
