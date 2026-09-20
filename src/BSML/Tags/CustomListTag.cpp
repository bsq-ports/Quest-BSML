#include "BSML/Tags/CustomListTag.hpp"
#include "BSML-Lite/Creation/Lists.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"

namespace BSML {
    static BSMLNodeParser<CustomListTag> customListTagParser({"custom-list"});

    UnityEngine::GameObject* CustomListTag::CreateObject(UnityEngine::Transform* parent) const {
        return BSML::Lite::CreateCustomList(parent, bsmlString);
    }

    void CustomListTag::parse(const tinyxml2::XMLElement& elem) {
        BSMLTag::parse(elem);
        INFO("Getting BSML string contents");

        bsmlString = "";
        auto handle = tinyxml2::XMLConstHandle(elem).FirstChildElement();
        for (
            const tinyxml2::XMLElement* element = nullptr;
            (element = handle.ToElement()) != nullptr;
            handle = handle.NextSiblingElement()
        ) {
            tinyxml2::XMLPrinter printer(nullptr , true);
            element->Accept(&printer);
            bsmlString += std::string_view(printer.CStr(), printer.CStrSize());
        }

        INFO("Got BSML string: {}", bsmlString);
    }

    void CustomListTag::HandleChildren(UnityEngine::Transform* parent, BSMLParserParams& parserParams, std::vector<std::unique_ptr<ComponentTypeWithData>>& componentInfo) const {
        // intentionally not doing anything
    }
}
