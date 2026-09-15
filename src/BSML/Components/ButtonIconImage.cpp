#include "BSML/Components/ButtonIconImage.hpp"
#include "Helpers/utilities.hpp"
#include "Helpers/delegates.hpp"
#include "HMUI/ImageView.hpp"

DEFINE_TYPE(BSML, ButtonIconImage);

void BSML::ButtonIconImage::OnEnable() {
    if (!button) return;

    if (!selectionStateChanged)
        selectionStateChanged = MakeSystemAction<HMUI::NoTransitionsButton::SelectionState>(this, ___OnSelectionStateDidChange_MethodRegistrator.info);
    button->add_selectionStateDidChangeEvent(selectionStateChanged);
    OnSelectionStateDidChange(button->get_selectionState());
}

void BSML::ButtonIconImage::OnDisable() {
    if (button && selectionStateChanged)
        button->remove_selectionStateDidChangeEvent(selectionStateChanged);
}

void BSML::ButtonIconImage::OnSelectionStateDidChange(HMUI::NoTransitionsButton::SelectionState state) {
    if (!image) return;
    auto color = image->get_color();
    color.a = state == HMUI::NoTransitionsButton::SelectionState::Disabled ? 0.25f : 1.0f;
    image->set_color(color);
}

void BSML::ButtonIconImage::SetIcon(StringW path) {
    if (image) Utilities::SetImage(image, path);
}

void BSML::ButtonIconImage::SetIcon(UnityEngine::Sprite* sprite) {
    if (image) image->set_sprite(sprite);
}

void BSML::ButtonIconImage::SetSkew(float value) {
    auto imageView = UnityW<UnityEngine::UI::Image>(image).try_cast<HMUI::ImageView>();
    if (!imageView) return;
    imageView->_skew = value;
    imageView->SetVerticesDirty();
}

void BSML::ButtonIconImage::SetUnderlineActive(bool active) {
    if (underline) underline->SetActive(active);
}
