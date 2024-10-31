#pragma once

#include <string>
#include <imgui.h>
#include <DirectXMath.h>
#include <memory>

namespace march
{
    class GfxTexture;

    class EditorGUI final
    {
    public:
        static void PrefixLabel(const std::string& label, const std::string& tooltip);

        static bool IntField(const std::string& label, const std::string& tooltip, int v[1], float speed = 1.0f, int min = 0, int max = 0);
        static bool FloatField(const std::string& label, const std::string& tooltip, float v[1], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Vector2Field(const std::string& label, const std::string& tooltip, float v[2], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Vector3Field(const std::string& label, const std::string& tooltip, float v[3], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool Vector4Field(const std::string& label, const std::string& tooltip, float v[4], float speed = 0.1f, float min = 0.0f, float max = 0.0f);
        static bool ColorField(const std::string& label, const std::string& tooltip, float v[4]);

        static bool FloatSliderField(const std::string& label, const std::string& tooltip, float v[1], float min, float max);

        static bool CollapsingHeader(const std::string& label, bool defaultOpen = false);
        static bool Combo(const std::string& label, const std::string& tooltip, int* currentItem, const std::string& itemsSeparatedByZeros);
        static bool CenterButton(const std::string& label, float width = 0.0f);
        static void Space();
        static void SeparatorText(const std::string& label);
        static bool TextField(const std::string& label, const std::string& tooltip, std::string& text, const std::string& charBlacklist);
        static bool Checkbox(const std::string& label, const std::string& tooltip, bool& value);
        static void BeginDisabled(bool disabled = true);
        static void EndDisabled();
        static void LabelField(const std::string& label1, const std::string& tooltip, const std::string& label2);
        static void PushID(const std::string& id);
        static void PushID(int id);
        static void PopID();
        static bool Foldout(const std::string& label, const std::string& tooltip, bool defaultOpen = false);
        static bool FoldoutClosable(const std::string& label, const std::string& tooltip, bool* pVisible);
        static void Indent(std::uint32_t count = 1);
        static void Unindent(std::uint32_t count = 1);
        static void SameLine(float offsetFromStartX = 0.0f, float spacing = -1.0f);
        static DirectX::XMFLOAT2 GetContentRegionAvail();
        static void SetNextItemWidth(float width);
        static void Separator();
        static bool BeginPopup(const std::string& id);
        // only call EndPopup() if BeginPopupXXX() returns true!
        static void EndPopup();
        static bool MenuItem(const std::string& label, bool selected = false, bool enabled = true);
        static bool BeginMenu(const std::string& label, bool enabled = true);
        // only call EndMenu() if BeginMenu() returns true!
        static void EndMenu();
        static void OpenPopup(const std::string& id);
        static bool FloatRangeField(const std::string& label, const std::string& tooltip, float& currentMin, float& currentMax, float speed = 0.1f, float min = 0.0f, float max = 0.0f);

        static bool BeginTreeNode(const std::string& label, bool isLeaf = false, bool openOnArrow = false, bool openOnDoubleClick = false, bool selected = false, bool showBackground = false, bool defaultOpen = false, bool spanWidth = true);
        // only call EndTreeNode() if BeginTreeNode() returns true!
        static void EndTreeNode();

        static ImGuiTreeNodeFlags GetTreeNodeFlags(bool isLeaf, bool openOnArrow, bool openOnDoubleClick, bool selected, bool showBackground, bool defaultOpen, bool spanWidth);
        static bool IsTreeNodeOpen(const std::string& id);

        enum class ItemClickOptions
        {
            None = 0,
            IgnorePopup = 1 << 0,
            TreeNodeItem = 1 << 1,
            TreeNodeIsOpen = 1 << 2,
            TreeNodeIsLeaf = 1 << 3,
        };

        enum class ItemClickResult
        {
            False = 0,
            TreeNodeArrow = 1,
            True = 2,
        };

        static bool HasItemClickOptions(ItemClickOptions options, ItemClickOptions checkOptions);
        static ItemClickResult IsItemClicked(ImGuiMouseButton button = ImGuiMouseButton_Left, ItemClickOptions options = ItemClickOptions::None);
        static bool IsWindowClicked(ImGuiMouseButton button = ImGuiMouseButton_Left, bool ignorePopup = false);
        static bool BeginPopupContextWindow();
        static bool BeginPopupContextItem(const std::string& id = "");

        static void DrawTexture(GfxTexture* texture);
        static bool Button(const std::string& label);

        static void BeginGroup();
        static void EndGroup();
        static float CalcButtonWidth(const std::string& label);
        static DirectX::XMFLOAT2 GetItemSpacing();
        static float GetCursorPosX();
        static void SetCursorPosX(float localX);

        enum class MarchObjectState
        {
            Null = 0,
            Persistent = 1,
            Temporary = 2
        };

        static bool BeginAssetTreeNode(const std::string& label, const std::string& assetPath, const std::string& assetGuid, bool isLeaf = false, bool openOnArrow = false, bool openOnDoubleClick = false, bool selected = false, bool showBackground = false, bool defaultOpen = false, bool spanWidth = true);
        static bool MarchObjectField(const std::string& label, const std::string& tooltip, const std::string& type, const std::string& persistentPath, std::string& persistentGuid, MarchObjectState currentObjectState);

        static float GetCollapsingHeaderOuterExtend();

        static bool BeginMainMenuBar();
        // only call EndMainMenuBar() if BeginMainMenuBar() returns true!
        static void EndMainMenuBar();

        static bool BeginMainViewportSideBar(const std::string& name, ImGuiDir dir, float contentHeight);
        // always call EndMainViewportSideBar() after BeginMainViewportSideBar()
        static void EndMainViewportSideBar();

    public:
        static constexpr float MinLabelWidth = 140.0f;
        static constexpr float MaxFieldWidth = 320.0f;
        static constexpr char* DragDropPayloadType_AssetGuid = "ASSET_GUID";
    };
}
