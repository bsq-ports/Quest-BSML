#include "BSML/Tags/TextPageScrollViewTag.hpp"
#include "Helpers/getters.hpp"
#include "BSML/Components/ExternalComponents.hpp"
#include "BSML/Components/TextPageScrollViewRefresher.hpp"

#include "UnityEngine/Object.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/TextPageScrollView.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

namespace BSML {
    static BSMLNodeParser<TextPageScrollViewTag> textPageScrollViewTagParser({"text-page", "page"});

    HMUI::TextPageScrollView* get_scrollViewTemplate();

    UnityEngine::GameObject* TextPageScrollViewTag::CreateObject(UnityEngine::Transform* parent) const {

        HMUI::TextPageScrollView* scrollView = Helpers::GetDiContainer()->InstantiatePrefabForComponent<HMUI::TextPageScrollView*>(get_scrollViewTemplate(), parent);

        scrollView->set_name("BSMLTextScrollPageView");
        scrollView->set_enabled(true);

        TMPro::TextMeshProUGUI* textMesh = scrollView->_text;
        textMesh->set_text("Default Text");

        textMesh->get_gameObject()->AddComponent<BSML::TextPageScrollViewRefresher*>()->scrollView = scrollView;

        auto externalComponents = scrollView->get_gameObject()->AddComponent<ExternalComponents*>();

        externalComponents->Add(textMesh);

        return scrollView->get_gameObject();
    }
}
