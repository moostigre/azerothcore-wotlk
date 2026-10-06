DELETE FROM `command` WHERE `name` IN ('history account list', 'history account add', 'history account remove', 'history character list', 'history character add', 'history character remove');
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('history account list', 2, 'Syntax: .history account list $account [#beforeNoteId]\nShow the latest 10 account and character notes, including removed notes. Use the displayed cursor for older notes.'),
('history account add', 2, 'Syntax: .history account add $account $note\nAdd a moderation note (1-255 UTF-8 bytes, plain text).'),
('history account remove', 2, 'Syntax: .history account remove $account #noteId $reason\nMark a note as removed. Its text and removal audit remain in history.'),
('history character list', 2, 'Syntax: .history character list $character [#beforeNoteId]\nShow the current account history, including all its character notes. Notes remain on the original account after character transfers.'),
('history character add', 2, 'Syntax: .history character add $character $note\nAdd a note linked to this character, realm and current account (1-255 UTF-8 bytes, plain text).'),
('history character remove', 2, 'Syntax: .history character remove $character #noteId $reason\nMark a note for this character and realm as removed. Use history account remove for an account-wide note.');

DELETE FROM `acore_string` WHERE `entry` BETWEEN 35483 AND 35496;
INSERT INTO `acore_string` (`entry`, `content_default`) VALUES
(35483, 'History for account {} (ID: {}), newest first (server time):'),
(35484, 'No notes on this page.'),
(35485, '[ID: {}] {} - by {} (account: {}, character GUID: {}, realm: {})'),
(35486, '  Subject: {} (character GUID: {}, realm: {})'),
(35487, '  Note: {}'),
(35488, '  Removed: {} by {} (account: {}, character GUID: {}, realm: {})'),
(35489, '  Removal reason: {}'),
(35490, 'Older notes: .history account list {} {}'),
(35491, 'Note added. Use .history account list {} to see it.'),
(35492, 'Note {} is marked as removed.'),
(35493, 'No matching note for this target.'),
(35494, 'This note is already removed.'),
(35495, 'Use 1-255 UTF-8 bytes of plain text, with no control characters or chat markup.'),
(35496, 'The history database operation failed. Check the server log.');
