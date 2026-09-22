#include "BSML/ViewControllers/BSMLViewController.hpp"
#include "BSML/MainThreadScheduler.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Canvas.hpp"
#include "HMUI/TextPageScrollView.hpp"
#include "HMUI/VerticalScrollIndicator.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "logging.hpp"
#include <memory>

namespace BSML {
    BSMLViewController* UpdateViewControllerPreview(BSMLViewController*, UnityEngine::Transform*, bool);

    void RunTextPageScrollTests() {
        auto root = UnityEngine::GameObject::New_ctor("BSMLScrollTimingTest");
        auto rect = root->AddComponent<UnityEngine::RectTransform*>();
        rect->set_sizeDelta({110, 70});
        auto view = UpdateViewControllerPreview(nullptr, rect, true);
        auto page = view->contentObject->GetComponentInChildren<HMUI::TextPageScrollView*>();
        auto failures = std::make_shared<int>(0);
        auto check = [page, failures](const char* phase, bool overflow) {
            bool actualOverflow = page->get_scrollableSize() > 0.01f;
            bool visible = page->_verticalScrollIndicator->get_gameObject()->get_activeSelf();
            bool pass = actualOverflow == overflow && visible == overflow;
            if (!pass) ++*failures;
            INFO("SCROLL-TEST {}: {} (overflow={} visible={})", pass ? "PASS" : "FAIL", phase, actualOverflow, visible);
        };
        auto measure = [page](const char* phase) {
            auto indicator = page->_verticalScrollIndicator;
            INFO("SCROLL-TEST {}: viewport={} content={} preferred={} scrollable={} indicatorActive={} handleActive={} normalized={}",
                phase, page->_viewport->get_rect().get_height(), page->get_contentSize(),
                page->_text->get_preferredHeight(), page->get_scrollableSize(),
                indicator->get_gameObject()->get_activeSelf(), indicator->_handle->get_gameObject()->get_activeSelf(),
                indicator->get_normalizedPageHeight());
        };
        UnityEngine::Canvas::ForceUpdateCanvases();
        measure("initial layout");
        MainThreadScheduler::ScheduleAfterTime(1.0f, [rect, page, measure, check] {
            measure("settled before manual refresh");
            check("short text fits", false);
            page->SetText(page->_text->get_text());
            page->RefreshButtons();
            measure("after manual refresh");
            rect->set_sizeDelta({110, 10});
            UnityEngine::Canvas::ForceUpdateCanvases();
            measure("shrunk during layout");
        });
        MainThreadScheduler::ScheduleAfterTime(2.0f, [rect, page, measure, check] {
            measure("shrunk settled");
            check("shrink shows indicator", true);
            page->SetText(page->_text->get_text());
            page->RefreshButtons();
            measure("shrunk manual refresh");
            rect->set_sizeDelta({110, 70});
            UnityEngine::Canvas::ForceUpdateCanvases();
            measure("expanded during layout");
        });
        MainThreadScheduler::ScheduleAfterTime(3.0f, [page, measure, check] {
            measure("expanded settled");
            check("expansion hides indicator", false);
            page->SetText(page->_text->get_text());
            page->RefreshButtons();
            measure("expanded manual refresh");
            std::string longText;
            for (int i = 0; i < 50; ++i) longText += "Long error message line\n";
            page->SetText(longText);
        });
        MainThreadScheduler::ScheduleAfterTime(4.0f, [page, measure, check] {
            measure("long text settled");
            check("long text shows indicator", true);
            page->SetText("Short error");
        });
        MainThreadScheduler::ScheduleAfterTime(5.0f, [root, rect, measure, check] {
            measure("short text settled");
            check("short replacement hides indicator", false);
            root->SetActive(false);
            rect->set_sizeDelta({110, 10});
            root->SetActive(true);
        });
        MainThreadScheduler::ScheduleAfterTime(6.0f, [root, measure, check, failures] {
            measure("reenabled settled");
            check("reenabling refreshes indicator", true);
            INFO("SCROLL-TEST RESULT: {} passed, {} failed", 6 - *failures, *failures);
            UnityEngine::Object::Destroy(root);
        });
    }
}
