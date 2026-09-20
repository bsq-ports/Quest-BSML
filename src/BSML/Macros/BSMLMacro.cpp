#include "BSML/Macros/BSMLMacro.hpp"

#include "BSML/Parsing/BSMLParserParams.hpp"
#include "BSML/ComponentTypeWithData.hpp"

namespace BSML {
    BSMLMacro::BSMLMacro() : BSMLNode() {
        #if MAKE_DOCS
        nodeType = NodeType::Macro;
        #endif
    };

    const BSMLMacro::PropMap& BSMLMacro::get_cachedProps() const {
        if (cachedProps.empty())
            const_cast<BSMLMacro*>(this)->cachedProps = get_props();
        return cachedProps;
    }

    void BSMLMacro::Handle(UnityEngine::Transform* parent, BSMLParserParams& parserParams, std::vector<std::unique_ptr<ComponentTypeWithData>>& componentInfo) const {
        auto data = ComponentTypeWithData::GetParameters(attributes, parserParams, get_cachedProps());
        Execute(parent, data, parserParams, componentInfo);
    }
}
