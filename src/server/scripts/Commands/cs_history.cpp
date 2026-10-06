/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CharacterCache.h"
#include "Chat.h"
#include "CommandScript.h"
#include "DatabaseEnv.h"
#include "Language.h"
#include "Player.h"
#include "RBAC.h"
#include "Realm.h"
#include "Timer.h"
#include "Util.h"
#include "WorldSession.h"
#include <algorithm>
#include <limits>

using namespace Acore::ChatCommands;

namespace
{
    constexpr uint32 HistoryPageSize = 10;
    constexpr std::size_t HistoryTextLimit = 255;

    struct HistoryTarget
    {
        uint32 AccountId;
        uint32 CharacterGuid = 0;
        std::string CharacterName;
        std::string AccountName;
    };

    HistoryTarget CharacterHistoryTarget(PlayerIdentifier const& player)
    {
        uint32 accountId = player.IsConnected() ? player.GetConnectedPlayer()->GetSession()->GetAccountId() :
            sCharacterCache->GetCharacterAccountIdByGuid(player.GetGUID());
        return {accountId, player.GetGUID().GetCounter(), player.GetName(), {}};
    }

    bool CheckHistoryTarget(ChatHandler* handler, HistoryTarget& target, bool write)
    {
        auto* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_HISTORY_TARGET);
        stmt->SetData(0, target.AccountId);
        PreparedQueryResult result = LoginDatabase.Query(stmt);
        if (!result)
        {
            handler->SendErrorMessage(LANG_HISTORY_DATABASE_ERROR);
            return false;
        }

        target.AccountName = (*result)[0].Get<std::string>();
        // History is shared across realms, so protect the target's highest staff rank.
        if (WorldSession* session = handler->GetSession())
        {
            uint64 targetSecurity = (*result)[1].Get<uint64>();
            if (session->GetSecurity() < targetSecurity || (write && session->GetSecurity() == targetSecurity))
            {
                handler->SendErrorMessage(LANG_YOURS_SECURITY_IS_LOW);
                return false;
            }
        }

        return true;
    }

    bool CheckHistoryText(ChatHandler* handler, std::string_view text)
    {
        std::wstring decoded;
        if (text.empty() || text.size() > HistoryTextLimit || text.find_first_not_of(' ') == std::string_view::npos ||
            !Utf8toWStr(text, decoded) || std::any_of(decoded.begin(), decoded.end(), [](wchar_t c)
            {
                return c < 0x20 || (c >= 0x7F && c <= 0x9F) || c == L'|' || c == 0x2028 || c == 0x2029;
            }))
        {
            handler->SendErrorMessage(LANG_HISTORY_INVALID_TEXT);
            return false;
        }

        return true;
    }

    // Escape names as well as notes before displaying stored data in chat.
    template<typename... Args>
    void HistoryMessage(ChatHandler* handler, uint32 entry, Args&&... args)
    {
        handler->SendSysMessage(handler->PGetParseString(entry, std::forward<Args>(args)...), true);
    }

    bool ListHistory(ChatHandler* handler, HistoryTarget target, Optional<uint64> before)
    {
        if (!CheckHistoryTarget(handler, target, false))
            return false;

        auto* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_HISTORY);
        stmt->SetData(0, target.AccountId);
        stmt->SetData(1, before.value_or(std::numeric_limits<uint64>::max()));
        PreparedQueryResult result = LoginDatabase.Query(stmt);
        HistoryMessage(handler, LANG_HISTORY_HEADER, target.AccountName, target.AccountId);
        if (!result)
        {
            handler->SendSysMessage(LANG_HISTORY_EMPTY);
            return true;
        }

        uint32 count = 0;
        uint64 lastId = 0;
        do
        {
            if (count++ == HistoryPageSize)
            {
                HistoryMessage(handler, LANG_HISTORY_NEXT, target.AccountName, lastId);
                break;
            }

            Field* fields = result->Fetch();
            lastId = fields[0].Get<uint64>();
            HistoryMessage(handler, LANG_HISTORY_ENTRY, lastId,
                Acore::Time::TimeToTimestampStr(Seconds(fields[7].Get<uint64>())),
                fields[6].Get<std::string>(), fields[4].Get<uint32>(), fields[5].Get<uint32>(), fields[1].Get<uint32>());
            if (fields[2].Get<uint32>())
                HistoryMessage(handler, LANG_HISTORY_CHARACTER, fields[3].Get<std::string>(),
                    fields[2].Get<uint32>(), fields[1].Get<uint32>());

            HistoryMessage(handler, LANG_HISTORY_NOTE, fields[8].Get<std::string>());
            if (!fields[9].IsNull())
            {
                HistoryMessage(handler, LANG_HISTORY_REMOVED,
                    Acore::Time::TimeToTimestampStr(Seconds(fields[9].Get<uint64>())), fields[13].Get<std::string>(),
                    fields[10].Get<uint32>(), fields[12].Get<uint32>(), fields[11].Get<uint32>());
                HistoryMessage(handler, LANG_HISTORY_REMOVAL_REASON, fields[14].Get<std::string>());
            }
        } while (result->NextRow());

        return true;
    }

    bool AddHistory(ChatHandler* handler, HistoryTarget target, std::string_view note)
    {
        if (!CheckHistoryTarget(handler, target, true) || !CheckHistoryText(handler, note))
            return false;

        WorldSession* session = handler->GetSession();
        Player* author = session ? session->GetPlayer() : nullptr;
        auto* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_HISTORY);
        stmt->SetData(0, target.AccountId);
        stmt->SetData(1, realm.Id.Realm);
        stmt->SetData(2, target.CharacterGuid);
        stmt->SetData(3, target.CharacterName);
        stmt->SetData(4, session ? session->GetAccountId() : 0u);
        stmt->SetData(5, author ? author->GetGUID().GetCounter() : 0u);
        stmt->SetData(6, author ? author->GetName() : "Console");
        stmt->SetData(7, std::string(note));
        if (!LoginDatabase.DirectExecute(stmt))
        {
            handler->SendErrorMessage(LANG_HISTORY_DATABASE_ERROR);
            return false;
        }

        HistoryMessage(handler, LANG_HISTORY_ADDED, target.AccountName);
        return true;
    }

    bool RemoveHistory(ChatHandler* handler, HistoryTarget target, uint64 id, std::string_view reason)
    {
        if (!CheckHistoryTarget(handler, target, true) || !CheckHistoryText(handler, reason))
            return false;

        auto* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_HISTORY_NOTE);
        stmt->SetData(0, target.AccountId);
        stmt->SetData(1, id);
        PreparedQueryResult result = LoginDatabase.Query(stmt);
        if (!result || (target.CharacterGuid && ((*result)[0].Get<uint32>() != target.CharacterGuid ||
            (*result)[1].Get<uint32>() != realm.Id.Realm)))
        {
            handler->SendErrorMessage(LANG_HISTORY_NOT_FOUND);
            return false;
        }

        if (!(*result)[2].IsNull())
        {
            handler->SendErrorMessage(LANG_HISTORY_ALREADY_REMOVED);
            return false;
        }

        WorldSession* session = handler->GetSession();
        Player* remover = session ? session->GetPlayer() : nullptr;
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_HISTORY_REMOVE);
        stmt->SetData(0, session ? session->GetAccountId() : 0u);
        stmt->SetData(1, realm.Id.Realm);
        stmt->SetData(2, remover ? remover->GetGUID().GetCounter() : 0u);
        stmt->SetData(3, remover ? remover->GetName() : "Console");
        stmt->SetData(4, std::string(reason));
        stmt->SetData(5, target.AccountId);
        stmt->SetData(6, id);
        stmt->SetData(7, (*result)[0].Get<uint32>());
        stmt->SetData(8, (*result)[1].Get<uint32>());
        // The NULL guard preserves the first removal if two realms remove the same note.
        if (!LoginDatabase.DirectExecute(stmt))
        {
            handler->SendErrorMessage(LANG_HISTORY_DATABASE_ERROR);
            return false;
        }

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_HISTORY_NOTE);
        stmt->SetData(0, target.AccountId);
        stmt->SetData(1, id);
        result = LoginDatabase.Query(stmt);
        if (!result || (*result)[2].IsNull())
        {
            handler->SendErrorMessage(LANG_HISTORY_DATABASE_ERROR);
            return false;
        }

        HistoryMessage(handler, LANG_HISTORY_REMOVE_SUCCESS, id);
        return true;
    }
}

class history_commandscript : public CommandScript
{
public:
    history_commandscript() : CommandScript("history_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable accountCommands =
        {
            { "list", HandleAccountList, rbac::RBAC_PERM_COMMAND_HISTORY_LIST, Console::Yes },
            { "add", HandleAccountAdd, rbac::RBAC_PERM_COMMAND_HISTORY_ADD, Console::Yes },
            { "remove", HandleAccountRemove, rbac::RBAC_PERM_COMMAND_HISTORY_REMOVE, Console::Yes }
        };
        static ChatCommandTable characterCommands =
        {
            { "list", HandleCharacterList, rbac::RBAC_PERM_COMMAND_HISTORY_LIST, Console::Yes },
            { "add", HandleCharacterAdd, rbac::RBAC_PERM_COMMAND_HISTORY_ADD, Console::Yes },
            { "remove", HandleCharacterRemove, rbac::RBAC_PERM_COMMAND_HISTORY_REMOVE, Console::Yes }
        };
        static ChatCommandTable historyCommands =
        {
            { "account", accountCommands },
            { "character", characterCommands }
        };
        static ChatCommandTable commandTable =
        {
            { "history", historyCommands }
        };
        return commandTable;
    }

    static bool HandleAccountList(ChatHandler* handler, AccountIdentifier account, Optional<uint64> before)
    {
        return ListHistory(handler, {account.GetID(), 0, {}, {}}, before);
    }

    static bool HandleAccountAdd(ChatHandler* handler, AccountIdentifier account, Tail note)
    {
        return AddHistory(handler, {account.GetID(), 0, {}, {}}, note);
    }

    static bool HandleAccountRemove(ChatHandler* handler, AccountIdentifier account, uint64 id, Tail reason)
    {
        return RemoveHistory(handler, {account.GetID(), 0, {}, {}}, id, reason);
    }

    static bool HandleCharacterList(ChatHandler* handler, PlayerIdentifier player, Optional<uint64> before)
    {
        return ListHistory(handler, CharacterHistoryTarget(player), before);
    }

    static bool HandleCharacterAdd(ChatHandler* handler, PlayerIdentifier player, Tail note)
    {
        return AddHistory(handler, CharacterHistoryTarget(player), note);
    }

    static bool HandleCharacterRemove(ChatHandler* handler, PlayerIdentifier player, uint64 id, Tail reason)
    {
        return RemoveHistory(handler, CharacterHistoryTarget(player), id, reason);
    }
};

void AddSC_history_commandscript()
{
    new history_commandscript();
}
