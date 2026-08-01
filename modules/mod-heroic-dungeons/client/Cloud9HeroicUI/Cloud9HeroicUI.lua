-- The 3.3.5a FrameXML hides the Dungeon Difficulty submenu below level 65
-- while the player is in Normal mode. The server supports heroic versions of
-- lower-level dungeons, so restore that stock menu entry after Blizzard has
-- applied its visibility rules.

local function ShowLowLevelDungeonDifficultyMenu()
    local dropdown = UIDROPDOWNMENU_INIT_MENU
    local menuLevel = UIDROPDOWNMENU_MENU_LEVEL

    if not dropdown or not dropdown.which or not menuLevel then
        return
    end

    local menu = UnitPopupMenus[dropdown.which]
    local shown = UnitPopupShown[menuLevel]
    if not menu or not shown then
        return
    end

    for index, value in ipairs(menu) do
        if value == "DUNGEON_DIFFICULTY" then
            shown[index] = 1
            return
        end
    end
end

hooksecurefunc("UnitPopup_HideButtons", ShowLowLevelDungeonDifficultyMenu)
