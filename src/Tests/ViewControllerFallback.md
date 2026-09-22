# View controller fallback (PC parity)

`BSML/ViewControllers/BSMLViewController.hpp` adds the Quest counterpart of PC's
`BSMLViewController.ParseWithFallback`. Derive from `BSML::BSMLViewController`
using `DECLARE_CLASS_CUSTOM` and supply a registered `get_Content` method:

```cpp
DECLARE_INSTANCE_METHOD(StringW, get_Content);
```

Return the page's BSML from that getter. The inherited `DidActivate` parses on
first activation. If overriding `DidActivate`, call the base implementation to
retain that behavior. Call `ParseWithFallback()` to rebuild later.

Each parse gets a full-size `Contents` child and uses the controller as its host.
On a standard C++ exception (including `BSML::ParseException`), the controller
logs the message, replaces the partial contents, and parses an **Invalid BSML**
page. Old contents are deactivated immediately and destroyed by Unity. As on
PC, provide `get_FallbackContent` to customize the error page; `{0}` receives
the XML-escaped exception message. A fallback error propagates, without recursive
fallback attempts. Destruction prevents subsequent parsing.

Existing mods using `HMUI::ViewController` and direct `parse_and_construct`
calls must adopt this base to use its exception handling. The low-level parser propagates conversion, malformed-XML and unknown-tag
errors; all three now reach the controller's fallback. The new base's default Content is empty; unlike PC's abstract
getter, it is concrete. Quest resolves the getter by name on the derived host
type because custom-types does not allocate new virtual slots for these getters.

Run **Component Test → View controller fallback (PC parity) → Run fallback
checks** to run `RunViewControllerTests()`. Use **Show parsing error** and
**Restore valid content** for a visible demonstration using the same controller.
The fixture verifies inherited activation, overridable
getters, content geometry, partial-content cleanup, the default and custom
fallbacks, XML escaping, content-getter errors, null content, recovery, direct
parser propagation, and the destruction guard.
