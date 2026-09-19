#include "BSML-Lite/ComponentCreation.hpp"

#include "BGLib/Polyglot/LocalizedTextMeshProUGUI.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/UI/LayoutGroup.hpp"

namespace BSML::Lite {
    TMPro::TextMeshProUGUI* StripLocalizedTextAndSet(UnityEngine::Transform* root, std::string_view relativePath, StringW text, BSML::ExternalComponents* externalComponents) {
        UnityEngine::Transform* target = root;
        if (!relativePath.empty()) {
            target = root->Find(relativePath);
            if (!target) return nullptr;
        }
        auto textObject = target->get_gameObject();

        if (auto localizer = textObject->GetComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>())
            UnityEngine::Object::Destroy(localizer);

        auto textMesh = textObject->GetComponent<TMPro::TextMeshProUGUI*>();
        if (!textMesh) return nullptr;

        textMesh->set_text(text);
        textMesh->set_richText(true);
        if (externalComponents) externalComponents->Add(textMesh);
        return textMesh;
    }

    void ApplyStandardSizingFixups(UnityEngine::GameObject* gameObject, UnityEngine::Transform* strayLayoutElementParent, UnityEngine::Component* searchLayoutGroupRoot, BSML::ExternalComponents* externalComponents) {
        if (strayLayoutElementParent) {
            if (auto stray = strayLayoutElementParent->GetComponent<UnityEngine::UI::LayoutElement*>())
                UnityEngine::Object::Destroy(stray);
        }

        auto sizeFitter = gameObject->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
        sizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        sizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        externalComponents->Add(sizeFitter);

        if (auto stackLayoutGroup = searchLayoutGroupRoot->GetComponentInChildren<UnityEngine::UI::LayoutGroup*>())
            externalComponents->Add(stackLayoutGroup);

        auto layoutElement = gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        if (!layoutElement) layoutElement = gameObject->AddComponent<UnityEngine::UI::LayoutElement*>();
        externalComponents->Add(layoutElement);
    }
}
