#include "BSML/Components/TextPageScrollViewRefresher.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

DEFINE_TYPE(BSML, TextPageScrollViewRefresher);

namespace BSML {
    void TextPageScrollViewRefresher::OnEnable() {
        if (scrollView && scrollView->m_CachedPtr.m_value) {
            scrollView->SetText(scrollView->_text->get_text());
            scrollView->RefreshButtons();
        }

    }

    void TextPageScrollViewRefresher::OnRectTransformDimensionsChange() {
        if (get_isActiveAndEnabled() && scrollView && scrollView->m_CachedPtr.m_value && !layoutCoroutine) {
            // Match PC: SetText can toggle objects, which is unsafe during a UI
            // rebuild. Coalesce dimension changes and refresh on the next frame.
            layoutCoroutine = StartCoroutine(custom_types::Helpers::CoroutineHelper::New(UpdateLayoutCoroutine()));
        }
    }

    custom_types::Helpers::Coroutine TextPageScrollViewRefresher::UpdateLayoutCoroutine() {
        co_yield nullptr;
        scrollView->SetText(scrollView->_text->get_text());
        scrollView->RefreshButtons();
        layoutCoroutine = nullptr;
    }
}
