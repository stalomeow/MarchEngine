#include "GraphicsDebuggerWindow.h"
#include "GfxSettings.h"
#include "EditorGUI.h"
#include "RenderDoc.h"
#include <string>

namespace march
{
    void GraphicsDebuggerWindow::OnDraw()
    {
        EditorGUI::SeparatorText("Settings");
        {
            EditorGUI::LabelField("Reversed Z", "", GfxSettings::UseReversedZBuffer ? "Yes" : "No");

            if constexpr (GfxSettings::ColorSpace == GfxColorSpace::Linear)
            {
                EditorGUI::LabelField("Color Space", "", "Linear");
            }
            else if constexpr (GfxSettings::ColorSpace == GfxColorSpace::Gamma)
            {
                EditorGUI::LabelField("Color Space", "", "Gamma");
            }
            else
            {
                EditorGUI::LabelField("Color Space", "", "Unknown");
            }
        }

        EditorGUI::Space();

        EditorGUI::SeparatorText("RenderDoc");
        {
            auto [major, minor, patch] = RenderDoc::GetVersion();

            EditorGUI::LabelField("Loaded", "", RenderDoc::IsLoaded() ? "Yes" : "No");
            EditorGUI::LabelField("Library", "", RenderDoc::GetLibraryPath());
            EditorGUI::LabelField("API Version", "", std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch));
            EditorGUI::LabelField("Num Captures", "", std::to_string(RenderDoc::GetNumCaptures()));
        }
    }
}
