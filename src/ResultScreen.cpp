#include "th_pch.h"

#include "Player.hpp"
#include "ResultScreen.hpp"
#include "ScreenEffect.hpp"
#include "pbg/Lzss.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

namespace th08
{

// FUNCTION: th08 0x453cd1
const char *ResultScreen::GetStageName(i32 stage)
{
    const char *name;

    if (stage >= 9)
    {
        name = (const char *)0x4b7108;
    }
    else
    {
        name = ((const char **)0x4c7fac)[stage];
    }
    return name;
}

// FUNCTION: th08 0x453cfa
const char *ResultScreen::GetCharacterName(i32 character)
{
    return ((const char **)0x4c7f4c)[character];
}

// FUNCTION: th08 0x453d0d
#pragma var_order(i, k, buffer, encrypted, offset, node, j, clrd, catk, pscr, vrsm, compressed, ptr, bytesToShift, xorValue, bytes, header)
void ResultScreen::WriteScore()
{
    i32 i;
    i32 k;
    u8 *buffer;
    u8 *encrypted;
    i32 offset;
    ScoreListNode *node;
    i32 j;
    Clrd *clrd;
    Catk *catk;
    Pscr *pscr;
    Vrsm vrsm;
    u8 *compressed;
    u8 *ptr;
    i32 bytesToShift;
    u8 xorValue;
    u8 *bytes;
    ScoreDat *header;

    offset = 0;
    buffer = (u8 *)g_ZunMemory.Alloc(0x640000, "d:\\cygwin\\home\\zun\\prog\\th08\\global.h");

    memcpy(buffer + offset, (void *)this->scoreData, 0x1c);
    offset += 0x1c;

    this->saveHeader.magic = TH8K_MAGIC;
    this->saveHeader.unkLen = 0xc;
    this->saveHeader.th8kLen = 0xc;
    this->saveHeader.version = 1;
    memcpy(buffer + offset, &this->saveHeader, sizeof(Th8k));
    offset += sizeof(Th8k);

    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 0xc; j++)
        {
            node = this->scores[i][j].next;

            for (k = 0; node != NULL && k < 0xa; k++)
            {
                u8 *data = (u8 *)node->data;
                if (*(u32 *)data == HSCR_MAGIC)
                {
                    data[0x14] = (u8)j;
                    data[0x15] = (u8)i;
                    *(u16 *)(data + 0x6) = 0x168;
                    *(u16 *)(data + 0x4) = 0x168;
                    data[0x8] = HSCR_VERSION;
                    data[0x9] &= 0;
                    memcpy(buffer + offset, data, 0x168);
                    offset += 0x168;
                }
                node = node->next;
            }
        }
    }

    clrd = (Clrd *)0x164b998;
    for (i = 0; i < 0xd; i++, clrd++)
    {
        clrd->base.magic = CLRD_MAGIC;
        clrd->base.unkLen = 0x24;
        clrd->base.th8kLen = 0x24;
        clrd->base.version = CLRD_VERSION;
        memcpy(buffer + offset, clrd, sizeof(Clrd));
        offset += sizeof(Clrd);
    }

    catk = (Catk *)0x160f548;
    for (i = 0; i < 0xde; i++, catk++)
    {
        if (catk->base.magic == CATK_MAGIC)
        {
            catk->spellcardNumber = (u16)i;
            catk->base.unkLen = 0x22c;
            catk->base.th8kLen = 0x22c;
            catk->base.version = CATK_VERSION;
            memcpy(buffer + offset, catk, sizeof(Catk));
            offset += sizeof(Catk);
        }
    }

    pscr = (Pscr *)0x164bb6c;
    for (i = 0; i < 0xc; i++, pscr++)
    {
        if (pscr->unk0x175 != 0)
        {
            memcpy(buffer + offset, pscr, sizeof(Pscr));
            offset += sizeof(Pscr);
        }
    }

    memcpy(buffer + offset, &this->saveNameData, sizeof(this->saveNameData));
    offset += sizeof(this->saveNameData);

    ((Flsp *)0x160f514)->base.magic = FLSP_MAGIC;
    ((Flsp *)0x160f514)->base.version = FLSP_VERSION;
    ((Flsp *)0x160f514)->base.unkLen = 0x20;
    ((Flsp *)0x160f514)->base.th8kLen = ((Flsp *)0x160f514)->base.unkLen;
    memcpy(buffer + offset, (void *)0x160f514, sizeof(Flsp));
    offset += sizeof(Flsp);

    Supervisor::UpdatePlayTime(&g_Supervisor);
    memcpy(buffer + offset, (void *)0x164cd0c, sizeof(Plst));
    offset += sizeof(Plst);

    vrsm.base.magic = VRSM_MAGIC;
    vrsm.base.version = 1;
    vrsm.base.unkLen = sizeof(Vrsm);
    vrsm.base.th8kLen = sizeof(Vrsm);
    ((u8 *)&vrsm.base)[9] &= 0;
    strcpy(vrsm.version, "0100d");
    vrsm.exeSize = *(i32 *)0x17ceab0;
    vrsm.exeChecksum = *(i32 *)0x17ceaac;
    memcpy(buffer + offset, &vrsm, sizeof(Vrsm));
    offset += sizeof(Vrsm);

    header = (ScoreDat *)buffer;
    header->decompressedFileSizeMinusHeader = offset - sizeof(ScoreDat);
    header->decompressedFileSize = offset;
    ptr = buffer;
    compressed = Lzss::Encode(buffer + sizeof(ScoreDat), ptr[0x14], (i32 *)&ptr[0x18]);

    memcpy(ptr, compressed, ptr[0x18]);
    g_ZunMemory.Free(compressed);
    offset = ptr[0x18] + sizeof(ScoreDat);

    header = (ScoreDat *)buffer;
    header->headerSize = sizeof(ScoreDat);
    header->checksum = 0;
    buffer[1] = (u8)g_Rng.GetRandomU16InRange(0x100);
    buffer[6] = (u8)g_Rng.GetRandomU16InRange(0x100);
    header->version = SCORE_DAT_VERSION;

    for (i = 4; i < offset; i++)
    {
        header->checksum = (u16)(header->checksum + buffer[i]);
    }

    bytes = buffer + 1;
    bytesToShift = offset - 2;
    xorValue = buffer[1];
    while (bytesToShift > 0)
    {
        u8 cur = bytes[1];
        xorValue = (u8)(((xorValue & 0xe0) >> 5) | ((xorValue & 0x1f) << 3));
        bytes[1] = (u8)(cur ^ xorValue);
        xorValue = (u8)(cur + xorValue);
        bytes++;
        bytesToShift--;
    }

    encrypted = (u8 *)FileSystem::Encrypt(buffer, offset, 0x59, 0x79, 0x100, 0xc00);
    FileSystem::WriteDataToFile("score.dat", encrypted, offset);
    g_ZunMemory.Free(buffer);
    g_ZunMemory.Free(encrypted);
}

static char *AppendFormat(char *buffer, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    return buffer + strlen(buffer);
}

// FUNCTION: th08 0x454298
#pragma var_order(cursor, i, j, spellIdx, spellNameBuf, node, timeBuf, spellNames, textBase)
void ResultScreen::LogScoreDataToFile(ResultScreen *resultScreen)
{
    char *cursor;
    i32 i;
    i32 j;
    i32 spellIdx;
    char spellNameBuf[0x40];
    ScoreListNode *node;
    char timeBuf[0x80];
    const char *spellNames[0xde];
    char *textBase;

    cursor = (char *)g_ZunMemory.Alloc(0x640000, "d:\\cygwin\\home\\zun\\prog\\th08\\global.h");
    textBase = cursor;
    memset(cursor, 0, 0x640000);

    cursor = AppendFormat(cursor, "# 東方永夜抄　〜 Imperishable Night. ver 1.00d 記録テキスト版\r\n# \r\n");
    cursor = AppendFormat(cursor, "# このファイルは、現在の記録をダンプした物です。\r\n");
    cursor = AppendFormat(cursor, "# このファイルの内容を変更してもゲームには反映されません。\r\n");
    cursor = AppendFormat(cursor, "# このファイルは自由に利用したり転載したり、別のフォーマットにしても構いません。\r\n");

    {
        time_t seconds;
        tm *timeinfo;

        time(&seconds);
        timeinfo = localtime(&seconds);
        strftime(timeBuf, 0x80, "%y/%m/%d %H:%M:%S", timeinfo);
    }

    cursor = AppendFormat(cursor, "# 　　　　　　　　　　　　   Time-stamp: <%s>\r\n", timeBuf);
    cursor = AppendFormat(cursor, "\r\n\r\n");
    cursor = AppendFormat(cursor, "Version: %.2f\r\n", (double)0.01f);
    cursor = AppendFormat(cursor, "\r\n");

    {
        Catk *catk;
        i32 m;

        catk = (Catk *)0x160f548;
        for (m = 0; m < 0xde; m++, catk++)
        {
            if (catk->base.magic == CATK_MAGIC)
            {
                spellNames[m] = catk->spellName;
            }
            else
            {
                spellNames[m] = (const char *)0x4b77d8;
            }
        }
    }

    for (i = 0; i < 5; i++)
    {
        i32 anyWritten = 0;
        i32 scoreCount = 0;

        for (j = 0; j < 0xc; j++)
        {
            i32 scoreWritten = 0;

            node = resultScreen->scores[i][j].next;
            while (node != NULL && scoreWritten < 0xa)
            {
                Hscr *data = node->data;
                if (data->base.magic == HSCR_MAGIC)
                {
                    cursor = AppendFormat(cursor, "  No.%d \r\n", scoreWritten + 1);
                    cursor = AppendFormat(cursor, "    得点  %d%d\r\n", data->score, data->numRetries);
                    cursor = AppendFormat(cursor, "    名前  %s\r\n", data->name);
                    if (data->stage == 0x63)
                    {
                        cursor = AppendFormat(cursor, "    ステージ         All Clear\r\n");
                    }
                    else
                    {
                        cursor = AppendFormat(cursor, "    ステージ         %s\r\n", GetStageName(data->stage));
                    }
                    cursor = AppendFormat(cursor, "    日付               %s\r\n", data->date);
                    cursor = AppendFormat(cursor, "    処理落ち率        %.2f％\r\n", data->lagPercentage);
                    cursor = AppendFormat(cursor, "    プレイ時間      %.2d分%.2d秒\r\n", data->playtimeFrames / 3600,
                                          (data->playtimeFrames / 60) % 60);
                    cursor = AppendFormat(cursor, "    初期プレイヤー数     %d人\r\n", ((u8 *)data)[0x44] + 1);
                    cursor = AppendFormat(cursor, "    得点アイテム数   %5d個\r\n", data->numPointItemsCollected);
                    cursor = AppendFormat(cursor, "    刻符数          %6d個\r\n", data->numTimeOrbsCollected);
                    cursor = AppendFormat(cursor, "    ミス回数           %3d回\r\n", data->numDeaths);
                    cursor = AppendFormat(cursor, "    ボム回数           %3d回\r\n", data->numBombsUsed);
                    cursor = AppendFormat(cursor, "    ラストスペル回数   %3d回\r\n", data->numLastSpells);
                    cursor = AppendFormat(cursor, "    ポーズ回数         %3d回\r\n", data->numPauses);
                    cursor = AppendFormat(cursor, "    コンティニュー回数 %3d回\r\n", data->numRetries);
                    cursor = AppendFormat(cursor, "    人間率          %3.2f%%\r\n", (double)data->humanityRate / 4.0f);
                    cursor = AppendFormat(cursor, "    取得スペルカード一覧 (総取得回数/総遭遇回数)\r\n");

                    for (spellIdx = 0; spellIdx < 0xde; spellIdx++)
                    {
                        if (data->spellCounters[spellIdx] != 0)
                        {
                            Catk *spell = (Catk *)(spellIdx * 0x22c + 0x160f548);
                            const char *name = spellNames[spellIdx];

                            memset(spellNameBuf, 0x20, 0x40);
                            spellNameBuf[0x30] = 0;
                            spellNameBuf[0x31] = 0;
                            memcpy(spellNameBuf, name, strlen(name));
                            cursor = AppendFormat(cursor, "      No.%.3d %s (%d/%d)\r\n", spellIdx + 1, spellNameBuf,
                                                  spell->inGameHistory.attempts[SHOT_ALL],
                                                  spell->inGameHistory.captures[SHOT_ALL]);
                        }
                    }
                    scoreCount++;
                }
                node = node->next;
                scoreWritten++;
            }

            if (scoreCount != 0)
            {
                if (anyWritten == 0)
                {
                    cursor = AppendFormat(cursor, "# ======================================== \r\n");
                    cursor = AppendFormat(cursor, "難易度 %s\r\n", ((const char **)0x4c7fd4)[i]);
                    anyWritten = 1;
                }
                cursor = AppendFormat(cursor, "# ---------------------------------------- \r\n");
                cursor = AppendFormat(cursor, "使用キャラ %s\r\n", GetCharacterName(j));
                cursor = AppendFormat(cursor, "%s\r\n", textBase);
            }
        }
    }

    cursor = AppendFormat(cursor, "# ======================================== \r\n");
    cursor = AppendFormat(cursor, "スペルカード一覧 本編　練習\r\n");

    {
        Catk *catk;
        i32 m;

        catk = (Catk *)0x160f548;
        for (m = 0; m < 0xde; m++, catk++)
        {
            if (catk->base.magic == CATK_MAGIC)
            {
                if (catk->WasAttemptedWithShot(SHOT_ALL))
                {
                    memset(spellNameBuf, 0x20, 0x40);
                    spellNameBuf[0x30] = 0;
                    spellNameBuf[0x31] = 0;
                    memcpy(spellNameBuf, catk->spellName, strlen(catk->spellName));
                    cursor = AppendFormat(cursor, "No.%.3d %s %3d/%3d %3d/%3d  (%s)\r\n", m + 1, spellNameBuf,
                                          catk->inGameHistory.captures[SHOT_ALL], catk->inGameHistory.attempts[SHOT_ALL],
                                          catk->spellPracticeHistory.captures[SHOT_ALL],
                                          catk->spellPracticeHistory.attempts[SHOT_ALL],
                                          ((const char **)0x4c7fd4)[((u8 *)catk)[0xf]]);
                }
            }
        }
    }

    Supervisor::UpdatePlayTime(&g_Supervisor);
    cursor = AppendFormat(cursor, "# ======================================== \r\n");
    cursor = AppendFormat(cursor, "総起動時間   %.2d:%.2d:%.2d\r\n", *(i32 *)0x164cd18, *(i32 *)0x164cd1c,
                          *(i32 *)0x164cd20);
    cursor = AppendFormat(cursor, "総プレイ時間 %.2d:%.2d:%.2d\r\n", *(i32 *)0x164cd28, *(i32 *)0x164cd2c,
                          *(i32 *)0x164cd30);
    cursor = AppendFormat(cursor, "プレイ回数　　　 　Easy 　Norm 　Hard 　Luna  Extra  Total\r\n");

    for (i = 0; i < 0xc; i++)
    {
        cursor = AppendFormat(cursor, "%s %6d %6d %6d %6d %6d %6d\r\n", GetCharacterName(i), *(i32 *)(i * 4 + 0x164cd3c),
                              *(i32 *)(i * 4 + 0x164cd80), *(i32 *)(i * 4 + 0x164cdc4), *(i32 *)(i * 4 + 0x164ce08),
                              *(i32 *)(i * 4 + 0x164ce4c), *(i32 *)(i * 4 + 0x164ced4));
    }

    cursor = AppendFormat(cursor, "%s %6d %6d %6d %6d %6d %6d\r\n", *(const char **)0x4c7f7c, *(i32 *)0x164cd38,
                          *(i32 *)0x164cd7c, *(i32 *)0x164cdc0, *(i32 *)0x164ce04, *(i32 *)0x164ce48,
                          *(i32 *)0x164ced0);
    cursor = AppendFormat(cursor, "クリア回数  　　 %6d %6d %6d %6d %6d %6d\r\n", *(i32 *)0x164cd70, *(i32 *)0x164cdb4,
                          *(i32 *)0x164cdf8, *(i32 *)0x164ce3c, *(i32 *)0x164ce80, *(i32 *)0x164cf08);
    cursor = AppendFormat(cursor, "コンティニュー   %6d %6d %6d %6d %6d %6d\r\n", *(i32 *)0x164cd74, *(i32 *)0x164cdb8,
                          *(i32 *)0x164cdfc, *(i32 *)0x164ce40, *(i32 *)0x164ce84, *(i32 *)0x164cec8);
    cursor = AppendFormat(cursor, "プラクティス　   %6d %6d %6d %6d %6d %6d\r\n", *(i32 *)0x164cd78, *(i32 *)0x164cdbc,
                          *(i32 *)0x164ce00, *(i32 *)0x164ce44, *(i32 *)0x164ce88, *(i32 *)0x164cf10);

    FileSystem::WriteDataToFile("score.txt", textBase, strlen(textBase));
    g_ZunMemory.Free(textBase);
}

// FUNCTION: th08 0x454c59
i32 ResultScreen::LinkScoreEx(void *out, int difficulty, i32 character)
{
    return ScoreDat::LinkScore(&this->scores[difficulty][character], (Hscr *)out);
}

// FUNCTION: th08 0x454c87
void ResultScreen::FreeScore(i32 difficulty, i32 character)
{
    ScoreDat::FreeAllScores(&this->scores[difficulty][character]);
}

// FUNCTION: th08 0x454cb2 (97% FIXME: switch 临时槽副本)
i32 ResultScreen::HandleCategorySelectScreen()
{
    i32 i;
    i32 selected;
    AnmVm *vm;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            vm = this->vms;

            for (i = 0; i < 0x48; i++, vm++)
            {
                vm->prefix.pendingInterrupt = 0x1;
            }

            for (i = 0; i <= 3; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 0x14)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        i = this->MoveCursor(4);

        if (i != 0)
        {
            for (i = 0; i <= 3; i++)
            {
                if (i == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0x2000))
        {
            this->LogScoreDataToFile(this);
        }

        if (WAS_PRESSED(0xa))
        {
            if (this->cursor == 3)
            {
                goto case3;
            }

            this->cursor = 3;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);

            for (i = 0; i <= 3; i++)
            {
                if (i == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0x1001))
        {
            vm = this->vms;
            selected = this->cursor;

            switch (selected)
            {
            case 0:
                this->SetState((ResultScreenState)3);

                for (i = 0; i <= 3; i++)
                {
                    if (i == this->cursor)
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x17;
                    }
                    else
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x1;
                    }
                }
                break;
            case 1:
                this->SetState((ResultScreenState)6);

                for (i = 0; i <= 3; i++)
                {
                    if (i == this->cursor)
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x17;
                    }
                    else
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x1;
                    }
                }
                break;
            case 2:
                for (i = 0; i <= 3; i++)
                {
                    if (i == this->cursor)
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x17;
                    }
                    else
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x1;
                    }
                }

                this->SetState((ResultScreenState)0x13);
                break;
            case3:
            case 3:
                this->SetState((ResultScreenState)2);
                g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
                return 1;
            }

            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 1;
        }
        break;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x4550b7
void ResultScreen::SetState(ResultScreenState state)
{
    this->previousScreenMode = this->screenMode;
    this->screenMode = state;
    this->stateCopy = state;
    this->subState = 0;
    this->subStateTimer = 0;
    this->screenTimer = 0;
    this->backPressed = 0;
}

// FUNCTION: th08 0x4550fc (99% FIXME: moveResult 槽/vmBase 死代码)
i32 ResultScreen::HandleHighScoreDifficultySelect()
{
    i32 i;
    i32 j;
    i32 k;
    i32 moveResult;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            this->cursor = this->savedDifficultyCursor;

            for (i = 4; i <= 8; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i - 4 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 6)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        moveResult = this->MoveCursor(5);

        if (moveResult != 0)
        {
            for (i = 4; i <= 8; i++)
            {
                if (i - 4 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0xa))
        {
            this->SetState((ResultScreenState)1);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->savedDifficultyCursor = this->cursor;
            this->cursor = 0;
            return 1;
        }

        if (WAS_PRESSED(0x1001))
        {
            for (i = 4; i <= 8; i++)
            {
                if (i - 4 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x17;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x1;
                }
            }

            this->savedDifficultyCursor = this->cursor;
            this->SetState((ResultScreenState)4);
            this->cursor = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 1;
        }
        break;
    }

    if (IS_PRESSED(0x4) || IS_PRESSED(0x100))
    {
        if (this->cheatCodeProgress < 4)
        {
            if (WAS_PRESSED(0x80))
            {
                this->cheatCodeProgress++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->cheatCodeProgress = 0;
            }
        }
        else if (this->cheatCodeProgress < 5)
        {
            if (WAS_PRESSED(0x40))
            {
                this->cheatCodeProgress++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->cheatCodeProgress = 0;
            }
        }
        else if (this->cheatCodeProgress < 7)
        {
            if (WAS_PRESSED(0x2000))
            {
                this->cheatCodeProgress++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->cheatCodeProgress = 0;
            }
        }
        else if (this->cheatCodeProgress < 0xa)
        {
            if (WAS_PRESSED(0x200))
            {
                this->cheatCodeProgress++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->cheatCodeProgress = 0;
            }
        }
        else
        {
            for (j = 0; j < 0xd; j++)
            {
                for (k = 0; k < 0x5; k++)
                {
                    *(u16 *)(j * 0x24 + k * 2 + 0x164b9a4) |= 0xffff;
                    *(u16 *)(j * 0x24 + k * 2 + 0x164b9ae) |= 0xffff;
                }
            }

            for (j = 0; j < 0xde; j++)
            {
                for (k = 0; k < 0xd; k++)
                {
                    (*(u32 *)(j * 0x22c + k * 4 + 0x160f66c))++;
                }
            }

            this->cheatCodeProgress = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_1UP, 0);
        }
    }
    else
    {
        this->cheatCodeProgress = 0;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x45567d
i32 ResultScreen::HandleHighScoreCharacterSelect()
{
    i32 i;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            this->cursor = this->savedCharacterCursor;

            for (i = 0xf; i <= 0x1a; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i - 0xf == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 6)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        i = this->MoveCursor(0xc);

        if (i != 0)
        {
            for (i = 0xf; i <= 0x1a; i++)
            {
                if (i - 0xf == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0xa))
        {
            this->SetState((ResultScreenState)3);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->savedCharacterCursor = this->cursor;

            for (i = 0xf; i <= 0x1a; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x1;
            }

            return 1;
        }

        if (WAS_PRESSED(0x1001))
        {
            AnmVm *vmBase = this->vms;

            for (i = 0xf; i <= 0x1a; i++)
            {
                if (i - 0xf == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x17;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x1;
                }
            }

            this->vms[0x28].prefix.pendingInterrupt = 0x3;
            this->savedCharacterCursor |= 0xffffffff;
            this->SetState((ResultScreenState)5);
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 1;
        }
        break;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x455925
i32 ResultScreen::HandleHighScoreScreen()
{
    if (this->savedCharacterCursor != this->cursor && this->screenTimer == 0xa)
    {
        this->savedCharacterCursor = this->cursor;
    }

    if (this->screenTimer < 6)
    {
        return 0;
    }

    i32 sel = this->cursor;

    if (this->MoveCursorHorizontally(0xc))
    {
        this->screenTimer = 0;
        this->vms[0x28].prefix.pendingInterrupt = (u16)(this->savedDifficultyCursor + 3);
        this->vms[sel + 0xf].prefix.pendingInterrupt = 0x18;
        this->vms[this->cursor + 0xf].prefix.pendingInterrupt = 0x19;
    }

    if (WAS_PRESSED(0xa))
    {
        this->savedCharacterCursor = this->cursor;
        this->SetState((ResultScreenState)4);
        this->vms[0x28].prefix.pendingInterrupt = 1;
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        return 1;
    }

    this->subStateTimer++;
    return 0;
}

// FUNCTION: th08 0x455a33
i32 ResultScreen::HandleSpellCardDifficultySelect()
{
    i32 i;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            this->cursor = this->savedSpellDifficultyCursor;

            for (i = 0x9; i <= 0xe; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i - 0x9 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 6)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        i = this->MoveCursor(6);

        if (i != 0)
        {
            for (i = 0x9; i <= 0xe; i++)
            {
                if (i - 0x9 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0xa))
        {
            this->SetState((ResultScreenState)1);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->savedSpellDifficultyCursor = this->cursor;
            this->cursor = 1;
            return 1;
        }

        if (WAS_PRESSED(0x1001))
        {
            AnmVm *vmBase = this->vms;

            for (i = 0x9; i <= 0xe; i++)
            {
                if (i - 0x9 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x17;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x1;
                }
            }

            this->savedSpellDifficultyCursor = this->cursor;
            this->SetState((ResultScreenState)7);
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 1;
        }
        break;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x455cb0
i32 ResultScreen::HandleSpellCardCharacterSelect()
{
    i32 i;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            this->cursor = this->shotTypeCursor;

            for (i = 0x1b; i <= 0x27; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i - 0x1b == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 6)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        i = this->MoveCursor(0xd);

        if (i != 0)
        {
            for (i = 0x1b; i <= 0x27; i++)
            {
                if (i - 0x1b == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0xa))
        {
            this->SetState((ResultScreenState)6);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->shotTypeCursor = this->cursor;

            for (i = 0x1b; i <= 0x27; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x1;
            }

            return 1;
        }

        if (WAS_PRESSED(0x1001))
        {
            AnmVm *vmBase = this->vms;

            for (i = 0x1b; i <= 0x27; i++)
            {
                if (i - 0x1b == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x17;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x1;
                }
            }

            this->shotTypeCursor = this->cursor;
            this->SetState((ResultScreenState)8);
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            this->vms[0x28].prefix.pendingInterrupt = 0x3;
            this->cursor = 0;
            this->savedSpellPageCursor |= 0xffffffff;
            return 1;
        }
        break;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x455f6b (93% FIXME: esi 缓存除数/or 展开/分支布局)
i32 ResultScreen::HandleSpellCardScreen()
{
    i32 spellcardCount;
    i32 spellIdx;
    i32 spellcardEntry;

    if (this->backPressed != 0 && this->screenTimer >= 0xa)
    {
        this->SetState((ResultScreenState)7);
    }

    spellcardCount = *(u32 *)(this->savedSpellDifficultyCursor * 4 + 0x4c67e8);

    if (this->savedSpellPageCursor != this->cursor || this->previousShotTypeCursor != this->shotTypeCursor)
    {
        if (this->screenTimer == 0xa)
        {
            this->savedSpellPageCursor = this->cursor;
            this->previousShotTypeCursor = this->shotTypeCursor;

            for (spellIdx = this->savedSpellPageCursor * 0xa; spellIdx < this->savedSpellPageCursor * 0xa + 0xa; spellIdx++)
            {
                if (spellIdx < spellcardCount)
                {
                    spellcardEntry = *(u32 *)(*(u32 *)(this->savedSpellDifficultyCursor * 4 + 0x4c67d0) + spellIdx * 4);

                    if (*(u32 *)(spellcardEntry * 0x22c + 0x160f69c) == 0)
                    {
                        g_AnmManager->DrawTextLeft(&this->scoreVms[spellIdx % 0xa], 0xffffff, 0, (const char *)0x4b77d8);
                    }
                    else
                    {
                        g_AnmManager->DrawTextLeft(&this->scoreVms[spellIdx % 0xa], 0xffffff, 0, (const char *)(spellcardEntry * 0x22c + 0x160f558));
                    }
                }

                this->scoreVms[spellIdx % 0xa].prefix.color1.a |= 0xff;
            }

            /* 0x64 + shotTypeCursor*4 + savedSpellDifficultyCursor*0x34：得分表数组索引。 */
            g_AnmManager->DrawTextLeft(&this->textVm, 0xffffff, 0, (const char *)0x4b795c, *(u32 *)((u8 *)this + this->savedSpellDifficultyCursor * 0x34 + 0x64 + this->shotTypeCursor * 4), spellcardCount);
            this->textVm.prefix.color1.a |= 0xff;
        }
    }

    if (this->screenTimer < 6)
    {
        return 0;
    }

    if (this->MoveCursorHorizontally((spellcardCount + 9) / 0xa))
    {
        this->screenTimer = 0;
        this->vms[0x28].prefix.pendingInterrupt = 0xa;
    }
    else if (this->MoveShotTypeCursor(0xd))
    {
        this->screenTimer = 0;
        this->shotCursorMoved = 1;
        this->vms[this->previousShotTypeCursor + 0x1b].prefix.pendingInterrupt = 0x18;
        this->vms[this->shotTypeCursor + 0x1b].prefix.pendingInterrupt = 0x19;
    }

    if (WAS_PRESSED(0xa))
    {
        this->backPressed = 1;
        this->screenTimer = 0;
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        this->vms[0x28].prefix.pendingInterrupt = 0x1;
        return 1;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x45621e
#pragma var_order(vm, i, charsLeft)
i32 ResultScreen::HandleResultKeyboard()
{
    AnmVm *vm;
    i32 i;
    i32 charsLeft;

    if (g_Supervisor.IsSlowModeEnabled() || g_Supervisor.IsClearBackBufferOnRefreshEnabled())
    {
        this->screenMode = RESULT_SCREEN_CONFIRM;
        this->screenTimer = 0;
        memcpy((void *)0x160f548, (const void *)0x162d770, 0x1e228);
        return 0;
    }

    if (this->screenTimer == 0)
    {
        this->savedCharacterCursor = (u8)(*(u8 *)0x164d0b1 + *(u8 *)0x164d0b2);
        this->savedDifficultyCursor = *(i32 *)0x160f538;

        vm = this->vms;
        for (i = 0; i < 0x48; i++, vm++)
        {
            vm->prefix.pendingInterrupt = (u16)(this->savedDifficultyCursor + 3);
        }

        g_AnmManager->DrawTextCentered(&this->scoreVms[0], 0xffffff, 0, GetCharacterName(this->savedCharacterCursor));
        this->scoreVms[0].prefix.color1.a |= 0xff;

        *(u32 *)0x164cf98 = *(u32 *)0x164d09c;
        *(u32 *)0x164cfb8 = (u32)(f32)((f32)*(i32 *)0x164d0a8 / *(i32 *)0x164d0ac * 100.0f);

        memcpy(&this->hscr, (const void *)0x164cf34, 0x168);
        this->hscr.score = *(i32 *)(*(i32 *)0x160f510 + 8);
        this->hscr.numRetries = *(u8 *)(*(i32 *)0x160f510 + 0x29);
        this->hscr.base.version = HSCR_VERSION;
        this->hscr.base.magic = HSCR_MAGIC;

        if (((*(u32 *)0x164d0b4 >> 4) & 1) != 0)
        {
            this->hscr.stage = 0x63;
        }
        else
        {
            this->hscr.stage = *(u8 *)0x164d2cc;
        }

        ((u8 *)&this->hscr)[9] = 1;
        strcpy(this->hscr.name, (const char *)((u8 *)this + 0x4645c));
        FormatDate(this->hscr.date);

        this->hscr.lagPercentage = (f32)((f32)(*(f32 *)0x17ce8e8 / *(f32 *)0x17ce8ec - 1.0f) * 2.0f);
        if (this->hscr.lagPercentage < 0.0f)
        {
            this->hscr.lagPercentage = 0.0f;
        }
        else if (this->hscr.lagPercentage > 1.0f)
        {
            this->hscr.lagPercentage = 1.0f;
        }
        this->hscr.lagPercentage = 1.0f - this->hscr.lagPercentage;

        i = this->LinkScoreEx(&this->hscr, this->savedDifficultyCursor, this->savedCharacterCursor);
        if (i < 0xa)
        {
            this->cursor = 0;
            if (this->nameEntryFlag != 0)
            {
                this->nameCursor = 0x5f;
            }
            strcpy(this->nameBuffer, (const char *)0x4b6cff);
        }
    }

    if (this->screenTimer < 0xa)
    {
        return 0;
    }

    if (WAS_PRESSED(0x10))
    {
        this->nameCursor -= 0x10;
        if (this->nameCursor < 0)
        {
            this->nameCursor += 0x60;
        }
        while (*(char *)(this->nameCursor + 0x4c7f48) == 0x20)
        {
            this->nameCursor -= 0x10;
            if (this->nameCursor < 0)
            {
                this->nameCursor += 0x60;
            }
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
    }

    if (WAS_PRESSED(0x20))
    {
        this->nameCursor += 0x10;
        if (this->nameCursor >= 0x60)
        {
            this->nameCursor -= 0x60;
        }
        while (*(char *)(this->nameCursor + 0x4c7f48) == 0x20)
        {
            this->nameCursor += 0x10;
            if (this->nameCursor >= 0x60)
            {
                this->nameCursor -= 0x60;
            }
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
    }

    if (WAS_PRESSED(0x40))
    {
        this->nameCursor--;
        if (this->nameCursor % 0x10 == 0xf)
        {
            this->nameCursor += 0x10;
        }
        if (this->nameCursor < 0)
        {
            this->nameCursor = 0xf;
        }
        while (*(char *)(this->nameCursor + 0x4c7f48) == 0x20)
        {
            this->nameCursor--;
            if (this->nameCursor % 0x10 == 0xf)
            {
                this->nameCursor += 0x10;
            }
            if (this->nameCursor < 0)
            {
                this->nameCursor = 0xf;
            }
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
    }

    if (WAS_PRESSED(0x80))
    {
        this->nameCursor++;
        if (this->nameCursor % 0x10 == 0)
        {
            this->nameCursor -= 0x10;
        }
        while (*(char *)(this->nameCursor + 0x4c7f48) == 0x20)
        {
            this->nameCursor++;
            if (this->nameCursor % 0x10 == 0)
            {
                this->nameCursor -= 0x10;
            }
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
    }

    if (WAS_PRESSED(0x1001))
    {
        charsLeft = this->cursor >= 8 ? 7 : this->cursor;
        i = charsLeft;

        if (this->nameCursor < 0x5e)
        {
            this->nameBuffer[i] = *(char *)(this->nameCursor + 0x4c7f48);
        }
        else if (this->nameCursor == 0x5e)
        {
            this->nameBuffer[i] = 0x20;
        }
        else
        {
            this->nameBuffer[i] = 0;
            goto confirm;
        }

        if (this->cursor < 8)
        {
            this->cursor++;
            if (this->cursor == 8)
            {
                this->nameCursor = 0x5f;
            }
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
    }

    if (WAS_PRESSED(0xa))
    {
        charsLeft = this->cursor >= 8 ? 7 : this->cursor;
        i = charsLeft;

        if (this->cursor > 0)
        {
            this->cursor--;
            this->nameBuffer[i] = 0x20;
            this->nameBuffer[this->cursor] = 0x20;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
    }

    if (WAS_PRESSED(0x8))
    {
        goto confirm;
    }

confirm:
    this->screenMode = RESULT_SCREEN_CONFIRM;
    this->screenTimer = 0;

    vm = this->vms;
    for (i = 0; i < 0x48; i++, vm++)
    {
        vm->prefix.pendingInterrupt = 2;
    }

    strcpy(this->nameBuffer, this->hscr.name);
    strcpy((char *)((u8 *)this + 0x4645c), this->nameBuffer);

    return 0;
}

// FUNCTION: th08 0x456938
void ResultScreen::FormatDate(char *buffer)
{
    time_t seconds;
    tm *timeinfo;

    time(&seconds);
    timeinfo = localtime(&seconds);
    strftime(buffer, 6, "%m/%d", timeinfo);
}

// FUNCTION: th08 0x45696f
#pragma var_order(vm, i, nextScreen)
i32 ResultScreen::HandleReplaySaveKeyboard()
{
    AnmVm *vm;
    i32 i;
    i32 nextScreen;

    nextScreen = this->screenMode;

    if (this->screenMode == RESULT_SCREEN_REPLAY_SAVE1)
    {
        if (this->screenTimer == 0xa)
        {
            if (g_Supervisor.IsSlowModeEnabled() || g_Supervisor.IsClearBackBufferOnRefreshEnabled())
            {
                nextScreen = RESULT_SCREEN_OTHER_STATS1;
            }
            else if (*(u8 *)(*(i32 *)0x160f510 + 0x29) != 0)
            {
                nextScreen = RESULT_SCREEN_REPLAY_SAVE5;
            }
            else
            {
                nextScreen = RESULT_SCREEN_REPLAY_SAVE2;
            }

            vm = (AnmVm *)((u8 *)this + 0xc30);
            for (i = 0; i < 0x48; i++, vm++)
            {
                vm->prefix.pendingInterrupt = (i16)nextScreen;
            }

            if (nextScreen != RESULT_SCREEN_REPLAY_SAVE2)
            {
                this->screenMode = RESULT_SCREEN_REPLAY_SAVE2;
            }
            this->cursor = 0;
        }

        vm = (AnmVm *)((u8 *)this + 0x85a8);
        if (this->cursor == 0)
        {
            vm->prefix.color1.a |= 0xff6060;
            *(u32 *)((u8 *)vm + 0x494) = (*(u32 *)((u8 *)vm + 0x494) & 0xff000000) | 0x606060;
        }
        else
        {
            vm->prefix.color1.a = (vm->prefix.color1.a & 0xff000000) | 0x606060;
            *(u32 *)((u8 *)vm + 0x494) = (*(u32 *)((u8 *)vm + 0x494) & 0xff000000) | 0xff6060;
        }

        if (this->screenTimer < 0xc)
        {
            return 0;
        }

        if (this->MoveCursorHorizontally(2) != 0)
        {
            /* highlight moved */
        }

        if (WAS_PRESSED(0xa))
        {
            this->screenMode = RESULT_SCREEN_REPLAY_SAVE3;
            this->screenTimer = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 0;
        }

        if (WAS_PRESSED(0x8))
        {
            this->screenMode = RESULT_SCREEN_REPLAY_SAVE2;
            this->screenTimer = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            return 0;
        }

        return 0;
    }

    if (this->screenMode == RESULT_SCREEN_REPLAY_SAVE2 || this->screenMode == RESULT_SCREEN_REPLAY_SAVE5)
    {
        if (this->screenTimer < 0xc)
        {
            return 0;
        }

        if (WAS_PRESSED(0xa))
        {
            this->screenMode = RESULT_SCREEN_REPLAY_SAVE3;
            this->screenTimer = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 0;
        }

        if (WAS_PRESSED(0x8))
        {
            this->screenMode = RESULT_SCREEN_CATEGORY;
            this->screenTimer = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            return 0;
        }

        return 0;
    }

    if (this->screenMode == RESULT_SCREEN_REPLAY_SAVE3)
    {
        /* overwrite confirmation */
        if (this->screenTimer >= 0xc)
        {
            if (WAS_PRESSED(0xa))
            {
                this->screenMode = RESULT_SCREEN_REPLAY_SAVE4;
                this->screenTimer = 0;
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                return 0;
            }
            if (WAS_PRESSED(0x8))
            {
                this->screenMode = RESULT_SCREEN_REPLAY_SAVE2;
                this->screenTimer = 0;
                g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
                return 0;
            }
        }
        return 0;
    }

    if (this->screenMode == RESULT_SCREEN_REPLAY_SAVE4)
    {
        /* actually save the replay */
        if (this->screenTimer >= 0xc)
        {
            if (WAS_PRESSED(0xa))
            {
                this->screenMode = RESULT_SCREEN_OTHER_STATS1;
                this->screenTimer = 0;
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                return 0;
            }
            if (WAS_PRESSED(0x8))
            {
                this->screenMode = RESULT_SCREEN_REPLAY_SAVE2;
                this->screenTimer = 0;
                g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
                return 0;
            }
        }
        return 0;
    }

    return 0;
}

#pragma var_order(menuTimerField, state)
ZunResult ResultScreen::CheckConfirmButton()
{
    u16 *menuTimerField;
    i32 state;

    state = this->screenMode;

    if (state == 0xf)
    {
        goto case_f;
    }
    if (state == 0x10)
    {
        goto case_10;
    }
    goto end;

case_f:
    if (this->screenTimer <= 0x1e)
    {
        menuTimerField = (u16 *)&this->vms[0x47];
        menuTimerField[0xff] = 0x12;
    }

    if (this->screenTimer >= 0x5a && WAS_PRESSED(0x1001))
    {
        menuTimerField = (u16 *)&this->vms[0x47];
        menuTimerField[0xff] = 0x2;
        this->screenTimer = 0;
        this->screenMode = 0x10;
    }
    goto end;

case_10:
    if (this->screenTimer >= 0x1e)
    {
        this->screenTimer = 9;
        this->screenMode = 0xa;
    }

end:
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x4578aa
#pragma var_order(vm, pos, charIdx, fadeIdx, fadeIdx2, self, screenMode)
i32 ResultScreen::HandleOtherStatsScreen()
{
    AnmVm *vm;
    Float3 pos;
    i32 charIdx;
    i32 fadeIdx;
    i32 fadeIdx2;
    ResultScreen *self;
    i32 screenMode;

    self = this;
    screenMode = self->screenMode;

    if (screenMode == RESULT_SCREEN_OTHER_STATS1)
    {
        if (self->screenTimer == 1)
        {
            pos.x = 56.0f;
            pos.y = 64.0f;
            pos.z = 0.0f;
            vm = self->scoreVms;
            vm->pos = pos;

            Supervisor::UpdatePlayTime(&g_Supervisor);
            g_AnmManager->DrawTextLeft(vm, 0xffffff, 0, "総起動時間   %.2d:%.2d:%.2d", *(i32 *)0x164cd18,
                                       *(i32 *)0x164cd1c, *(i32 *)0x164cd20);
            self->lastTotalSeconds = (u8)*(i32 *)0x164cd20;
            vm++;
            pos.y += 17.0f;
            vm->pos = pos;

            Supervisor::UpdatePlayTime(&g_Supervisor);
            g_AnmManager->DrawTextLeft(vm, 0xffffff, 0, "総プレイ時間 %.2d:%.2d:%.2d", *(i32 *)0x164cd28,
                                       *(i32 *)0x164cd2c, *(i32 *)0x164cd30);
            vm++;
            pos.y += 17.0f;
            vm->pos = pos;

            g_AnmManager->DrawTextLeft(vm, 0xffffff, 0, "プレイ回数　　　 　Easy 　Norm 　Hard 　Luna  Extra  Total");

            for (charIdx = 0; charIdx < 0xc; charIdx++)
            {
                vm++;
                pos.y += 17.0f;
                vm->pos = pos;

                g_AnmManager->DrawTextLeft(vm, 0xffffff, 0, "%s %6d %6d %6d %6d %6d %6d",
                                           ((const char **)0x4c7f4c)[charIdx], *(i32 *)(charIdx * 4 + 0x164cd3c),
                                           *(i32 *)(charIdx * 4 + 0x164cd80), *(i32 *)(charIdx * 4 + 0x164cdc4),
                                           *(i32 *)(charIdx * 4 + 0x164ce08), *(i32 *)(charIdx * 4 + 0x164ce4c),
                                           *(i32 *)(charIdx * 4 + 0x164ced4));
            }

            vm++;
            pos.y += 17.0f;
            vm->pos = pos;

            g_AnmManager->DrawTextLeft(vm, 0xffffff, 0, "%s %6d %6d %6d %6d %6d %6d", *(const char **)0x4c7f7c,
                                       *(i32 *)0x164cd38, *(i32 *)0x164cd7c, *(i32 *)0x164cdc0, *(i32 *)0x164ce04,
                                       *(i32 *)0x164ce48, *(i32 *)0x164ced0);
            vm++;
            pos.y += 34.0f;
            vm->pos = pos;

            g_AnmManager->DrawTextLeft(vm, 0xffffff, 0, "クリア回数  　　 %6d %6d %6d %6d %6d %6d", *(i32 *)0x164cd70,
                                       *(i32 *)0x164cdb4, *(i32 *)0x164cdf8, *(i32 *)0x164ce3c, *(i32 *)0x164ce80,
                                       *(i32 *)0x164cf08);
            vm++;
            pos.y += 17.0f;
            vm->pos = pos;

            g_AnmManager->DrawTextLeft(vm, 0xffffff, 0, "コンティニュー   %6d %6d %6d %6d %6d %6d", *(i32 *)0x164cd74,
                                       *(i32 *)0x164cdb8, *(i32 *)0x164cdfc, *(i32 *)0x164ce40, *(i32 *)0x164ce84,
                                       *(i32 *)0x164cec8);
            vm++;
            pos.y += 17.0f;
            vm->pos = pos;

            g_AnmManager->DrawTextLeft(vm, 0xffffff, 0, "プラクティス　   %6d %6d %6d %6d %6d %6d", *(i32 *)0x164cd78,
                                       *(i32 *)0x164cdbc, *(i32 *)0x164ce00, *(i32 *)0x164ce44, *(i32 *)0x164ce88,
                                       *(i32 *)0x164cf10);
        }

        if (self->screenTimer >= 0x28)
        {
            self->screenMode = RESULT_SCREEN_OTHER_STATS2;
        }
        else
        {
            vm = self->scoreVms;
            for (fadeIdx = 0; fadeIdx < 0x14; fadeIdx++, vm++)
            {
                vm->prefix.color1.a = (u8)((self->screenTimer * 0xff) / 0x28);
            }
        }
        return 0;
    }

    if (screenMode == RESULT_SCREEN_OTHER_STATS2)
    {
        if (self->screenTimer % 60 == 0)
        {
            Supervisor::UpdatePlayTime(&g_Supervisor);

            if (self->lastTotalSeconds != (u8)*(i32 *)0x164cd20)
            {
                vm = self->scoreVms;
                g_AnmManager->DrawTextLeft(vm, 0xffffff, 0, "総起動時間   %.2d:%.2d:%.2d", *(i32 *)0x164cd18,
                                           *(i32 *)0x164cd1c, *(i32 *)0x164cd20);
                self->lastTotalSeconds = (u8)*(i32 *)0x164cd20;
            }
        }

        if (WAS_PRESSED(0x100b))
        {
            self->screenMode = RESULT_SCREEN_OTHER_STATS3;
            self->screenTimer = 0;
        }
        return 0;
    }

    if (screenMode == RESULT_SCREEN_OTHER_STATS3)
    {
        if (self->screenTimer < 0x14)
        {
            vm = self->scoreVms;
            for (fadeIdx2 = 0; fadeIdx2 < 0x14; fadeIdx2++, vm++)
            {
                vm->prefix.color1.a = (u8)(0xff - ((self->screenTimer * 0xff) / 0x14));
            }
        }
        else
        {
            self->screenMode = RESULT_SCREEN_CATEGORY_INIT;
            self->screenTimer = 0;
            return 1;
        }
        return 0;
    }

    return 0;
}

// FUNCTION: th08 0x457e00
#pragma var_order(pos, lagPercentage, screenMode)
i32 ResultScreen::DrawFinalStats()
{
    Float3 pos;
    f32 lagPercentage;
    i32 screenMode;
    i32 temp;

    screenMode = this->screenMode;
    if (screenMode < 0xf || screenMode > 0x10)
    {
        return 0;
    }

    g_AsciiManager.SetColor(*(u32 *)((u8 *)&this->vms[0x2c] + 0x1f0));

    lagPercentage = 0.0f;

    if (*(i32 *)0x160f538 < 4)
    {
        lagPercentage = (f32)*(i32 *)0x164d30c / *(f32 *)0x4b7b3c;
    }
    else
    {
        lagPercentage = (f32)*(i32 *)0x164d30c / *(f32 *)0x4b7b38;
    }

    pos = this->vms[0x2c].pos;
    pos.x += *(f32 *)0x4b7b34;
    pos.y += *(f32 *)0x4b42cc;

    g_AsciiManager.AddFormatText(&pos, (const char *)0x4b7b30, *(i32 *)(*(i32 *)0x160f510));
    pos.x += (f32)(*(i32 *)0x4d50a0 * 9);

    temp = *(u8 *)(*(i32 *)0x160f510 + 0x29);
    if (temp >= 0xa)
    {
        temp = 9;
    }
    g_AsciiManager.AddFormatText(&pos, (const char *)0x4b4ce4, temp);
    pos.x -= (f32)(*(i32 *)0x4d50a0 * 9);

    if (*(i32 *)(*(i32 *)0x160f510) < 0x1e8480)
    {
        lagPercentage -= *(f32 *)0x4b6e98;
    }
    else if (*(i32 *)(*(i32 *)0x160f510) < 0xbebc200)
    {
        lagPercentage += (f32)(*(i32 *)(*(i32 *)0x160f510) - 0x1e8480) / *(f32 *)0x4b7b2c * *(f32 *)0x4b430c - *(f32 *)0x4b6e98;
    }
    else
    {
        lagPercentage += *(f32 *)0x4b432c;
    }

    pos.y += *(f32 *)0x4b7b28;
    g_AsciiManager.AddString(&pos, ((const char **)0x4c7ffc)[*(i32 *)0x160f538]);
    lagPercentage += *(f32 *)(*(i32 *)0x160f538 * 4 + 0x4b7b14);
    pos.x += (f32)*(i32 *)0x4d50a0;
    pos.y += *(f32 *)0x4b7b28;

    if (((*(u32 *)0x164d0b4 >> 4) & 1) != 0)
    {
        g_AsciiManager.AddString(&pos, (const char *)0x4b7afc);
        lagPercentage += *(f32 *)0x4b4328;
    }
    else
    {
        if (lagPercentage <= *(f32 *)0x4b4338)
        {
            lagPercentage = *(f32 *)0x4b6944;
        }
        g_AsciiManager.AddFormatText(&pos, (const char *)0x4b7b08, (f64)(lagPercentage * *(f32 *)0x4b4980));
        lagPercentage += lagPercentage * *(f32 *)0x4b4328;
    }

    pos.y += *(f32 *)0x4b7b28;
    g_AsciiManager.AddFormatText(&pos, (const char *)0x4b7b30, *(u8 *)(*(i32 *)0x160f510 + 0x29));
    lagPercentage -= (f32)(*(u8 *)(*(i32 *)0x160f510 + 0x29) * *(f32 *)0x4b4390);

    pos.y += *(f32 *)0x4b7b28;
    g_AsciiManager.AddFormatText(&pos, (const char *)0x4b7b30, *(i32 *)(*(i32 *)0x160f510 + 0x1c));
    lagPercentage += (f32)(*(i32 *)(*(i32 *)0x160f510 + 0x1c) * *(f32 *)(*(i32 *)0x160f538 * 4 + 0x4c7fe8));

    lagPercentage = (f32)((f32)(*(f32 *)0x17ce8e8 / *(f32 *)0x17ce8ec - 1.0f) * 2.0f);
    if (lagPercentage < 0.0f)
    {
        lagPercentage = 0.0f;
    }
    else if (lagPercentage > 1.0f)
    {
        lagPercentage = 1.0f;
    }
    lagPercentage = 1.0f - lagPercentage;

    pos.y += *(f32 *)0x4b7b28;
    g_AsciiManager.AddFormatText(&pos, (const char *)0x4b7b08, (f64)lagPercentage);

    if (lagPercentage > *(f32 *)0x4b4530)
    {
        lagPercentage = *(f32 *)0x4b4c7c;
    }
    else
    {
        lagPercentage = lagPercentage * *(f32 *)0x4b4328 / *(f32 *)0x4b4980;
    }

    if (*(i32 *)(*(i32 *)0x160f510 + 0x30) < 0x320)
    {
        lagPercentage += (f32)*(i32 *)(*(i32 *)0x160f510 + 0x30) * *(f32 *)0x4b48d4;
    }
    else
    {
        lagPercentage += *(f32 *)0x4b4300;
    }

    if (*(i32 *)(*(i32 *)0x160f510 + 0xc) < 0x1388)
    {
        lagPercentage += (f32)*(i32 *)(*(i32 *)0x160f510 + 0xc) * *(f32 *)0x4b7af8;
    }
    else
    {
        lagPercentage += *(f32 *)0x4b7af4;
    }

    g_AsciiManager.SetColor(0xffffffff);

    return 0;
}

// FUNCTION: th08 0x4582a0
ZunResult ResultScreen::RegisterChain(u32 unk)
{
    ResultScreen *resultScreen =
        (ResultScreen *)g_ZunMemory.AddToRegistry(new ResultScreen(), sizeof(ResultScreen), "ResultSysInf");
    g_ScreenEffectCounter = 0;
    utils::GuiDebugPrint("Stg.PlayTimeAll = %d\r\n", g_164d30c);

    if (unk == 1)
    {
        if (!g_GameManager.IsPracticeMode())
        {
            resultScreen->screenMode = 9;
        }
        else if (((g_PlayerFlags >> 0xe) & 1) != 0)
        {
            resultScreen->screenMode = 0x16;
        }
        else
        {
            resultScreen->screenMode = 0x11;
        }
    }
    else if (unk == 2)
    {
        resultScreen->screenMode = 0x12;
        resultScreen->AddedCallback(resultScreen);
        return ZUN_SUCCESS;
    }

    resultScreen->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    resultScreen->calcChain->addedCallback = (ChainLifetimeCallback)AddedCallback;
    resultScreen->calcChain->deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    resultScreen->calcChain->arg = resultScreen;
    if (g_Chain.AddToCalcChain(resultScreen->calcChain, 0x10))
    {
        return ZUN_ERROR;
    }

    resultScreen->drawChain = g_Chain.CreateElem((ChainCallback)OnDraw);
    resultScreen->drawChain->arg = resultScreen;
    g_Chain.AddToDrawChain(resultScreen->drawChain, 0x12);

    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x4584b0
#pragma var_order(vmCase2, i, vm)
ChainCallbackResult ResultScreen::OnUpdate(ResultScreen *resultScreen)
{
    i32 screenMode;
    AnmVm *vm;
    i32 i;
    AnmVm *vmCase2;

    screenMode = resultScreen->screenMode;

    switch (screenMode)
    {
    case RESULT_SCREEN_PRACTICE_RESULT:
        *(i32 *)((u8 *)&g_Supervisor + 0x15c) = 1;
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    case RESULT_SCREEN_SPELL_PRACTICE_RESULT:
        *(i32 *)((u8 *)&g_Supervisor + 0x15c) = 1;
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    case RESULT_SCREEN_ONE_SHOT:
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    case RESULT_SCREEN_CATEGORY_INIT:
        resultScreen->SetState(RESULT_SCREEN_CATEGORY);
        resultScreen->HandleCategorySelectScreen();
        break;
    case RESULT_SCREEN_CATEGORY:
        resultScreen->HandleCategorySelectScreen();
        break;
    case RESULT_SCREEN_LOADING:
        if (resultScreen->screenTimer == 1)
        {
            Float3 loadingPos(500.0f, 440.0f, 0.0f);
            g_Supervisor.SetupLoadingVms(&loadingPos);

            vmCase2 = resultScreen->vms;
            for (i = 0; i < 0x48; i++, vmCase2++)
            {
                vmCase2->prefix.pendingInterrupt = 2;
            }
        }
        if (resultScreen->screenTimer >= 0x14)
        {
            *(i32 *)((u8 *)&g_Supervisor + 0x15c) = 1;
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        break;
    case RESULT_SCREEN_HIGH_SCORE_CHARACTER:
        resultScreen->HandleHighScoreCharacterSelect();
        break;
    case RESULT_SCREEN_HIGH_SCORE_DIFFICULTY:
        resultScreen->HandleHighScoreDifficultySelect();
        break;
    case RESULT_SCREEN_HIGH_SCORE:
        resultScreen->HandleHighScoreScreen();
        break;
    case RESULT_SCREEN_SPELL_CHARACTER:
        resultScreen->HandleSpellCardCharacterSelect();
        break;
    case RESULT_SCREEN_SPELL_DIFFICULTY:
        resultScreen->HandleSpellCardDifficultySelect();
        break;
    case RESULT_SCREEN_SPELL:
        resultScreen->HandleSpellCardScreen();
        break;
    case RESULT_SCREEN_RESULT:
        resultScreen->HandleResultKeyboard();
        break;
    case RESULT_SCREEN_REPLAY_SAVE1:
    case RESULT_SCREEN_REPLAY_SAVE2:
    case RESULT_SCREEN_REPLAY_SAVE3:
    case RESULT_SCREEN_REPLAY_SAVE4:
    case RESULT_SCREEN_REPLAY_SAVE5:
        resultScreen->HandleReplaySaveKeyboard();
        break;
    case RESULT_SCREEN_CONFIRM:
    case RESULT_SCREEN_CONFIRM2:
        resultScreen->CheckConfirmButton();
        break;
    case RESULT_SCREEN_OTHER_STATS1:
    case RESULT_SCREEN_OTHER_STATS2:
    case RESULT_SCREEN_OTHER_STATS3:
        if (resultScreen->HandleOtherStatsScreen() != 0)
        {
            resultScreen->SetState(RESULT_SCREEN_CATEGORY);
            resultScreen->HandleCategorySelectScreen();
        }
        break;
    }

    vm = resultScreen->vms;
    for (i = 0; i < 0x48; i++, vm++)
    {
        g_AnmManager->ExecuteScript(vm);
    }

    resultScreen->screenTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: th08 0x4586b4
#pragma var_order(vm, i, screenMode, pos, temp, temp2, temp3, temp4)
ChainCallbackResult ResultScreen::OnDraw(ResultScreen *resultScreen)
{
    AnmVm *vm;
    i32 i;
    i32 screenMode;
    Float3 pos;
    f32 temp;
    f32 temp2;
    i32 temp3;
    i32 temp4;
    Float3 pos2;

    vm = resultScreen->vms;
    for (i = 0; i < 0x48; i++, vm++)
    {
        pos = vm->pos;
        g_AnmManager->ExecuteScript(vm);
        vm->pos = pos;
    }

    g_AnmManager->ExecuteScript(&resultScreen->vms[0x40]);

    if (resultScreen->screenMode == RESULT_SCREEN_SPELL)
    {
        /* spell card list screen */
        i32 spellcardCount;
        i32 spellIdx;

        spellcardCount = *(i32 *)(resultScreen->savedSpellDifficultyCursor * 4 + 0x4c67e8);

        for (i = 0; i < 0xa; i++)
        {
            spellIdx = resultScreen->savedSpellPageCursor * 0xa + i;
            if (spellIdx < spellcardCount)
            {
                i32 spellcardEntry = *(i32 *)(*(i32 *)(resultScreen->savedSpellDifficultyCursor * 4 + 0x4c67d0) +
                                              spellIdx * 4);
                if (spellcardEntry * 0x22c + 0x160f69c == 0)
                {
                    g_AnmManager->DrawTextLeft(&resultScreen->scoreVms[i], 0xffffff, 0, (const char *)0x4b77d8);
                }
                else
                {
                    g_AnmManager->DrawTextLeft(&resultScreen->scoreVms[i], 0xffffff, 0,
                                               (const char *)(spellcardEntry * 0x22c + 0x160f558));
                }
            }
            resultScreen->scoreVms[i].prefix.color1.a |= 0xff;
        }
    }

    if (resultScreen->screenMode >= RESULT_SCREEN_REPLAY_SAVE1 && resultScreen->screenMode <= RESULT_SCREEN_REPLAY_SAVE5)
    {
        /* replay save screens - execute the slot vms */
        vm = (AnmVm *)((u8 *)resultScreen + 0x8304);
        for (i = 0; i < 6; i++, vm++)
        {
            g_AnmManager->ExecuteScript(vm);
        }
    }

    g_AsciiManager.SetColor(0xffffffff);

    if (resultScreen->screenMode == RESULT_SCREEN_RESULT || resultScreen->screenMode == RESULT_SCREEN_REPLAY_SAVE5)
    {
        /* draw the name entry grid */
        for (i = 0; i < 6; i++)
        {
            pos = resultScreen->vms[0x2c].pos;
            pos.y += (f32)(i * 0x14);
            pos2 = pos;

            for (i32 j = 0; j < 0x10; j++)
            {
                pos.x = resultScreen->vms[0x2c].pos.x;
                pos.x += (f32)(j * 0x14);

                if (i * 0x10 + j == resultScreen->nameCursor)
                {
                    g_AsciiManager.SetColor(0xc0c0c0c0);
                }
                else
                {
                    g_AsciiManager.SetColor(0xffffff00);
                }

                char c[2];
                c[0] = *(char *)(i * 0x10 + j + 0x4c7f48);
                c[1] = 0;
                g_AsciiManager.AddString(&pos2, c);
                pos2.x += 20.0f;
            }
        }
    }

    if (resultScreen->screenMode == RESULT_SCREEN_CONFIRM || resultScreen->screenMode == RESULT_SCREEN_CONFIRM2)
    {
        resultScreen->DrawFinalStats();
    }

    if (resultScreen->screenMode >= RESULT_SCREEN_OTHER_STATS1 && resultScreen->screenMode <= RESULT_SCREEN_OTHER_STATS3)
    {
        vm = resultScreen->scoreVms;
        for (i = 0; i < 0x14; i++, vm++)
        {
            g_AnmManager->ExecuteScript(vm);
        }
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: th08 0x45964d
#pragma var_order(vm, i, j, k, resultScreen, spellIdx, catk, pos, pos2)
ZunResult ResultScreen::AddedCallback(ResultScreen *resultScreen)
{
    AnmVm *vm;
    i32 i;
    i32 j;
    i32 k;
    i32 spellIdx;
    Catk *catk;
    Float3 pos;
    Float3 pos2;

    g_GameManager.IsPhantasmUnlocked();

    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 0xc; j++)
        {
            for (k = 0; k < 0xa; k++)
            {
                Hscr *entry = &resultScreen->hscrCache[i][j][k];

                entry->score = 100000 - k * 10000;
                entry->lagPercentage = 0.0f;
                entry->base.magic = *(u32 *)0x4b7d58;
                entry->difficulty = (u8)i;
                entry->base.version = HSCR_VERSION;
                entry->base.unkLen = 0x168;
                entry->base.th8kLen = 0x168;
                entry->stage = 0;
                ((u8 *)entry)[9] &= 0;
                entry->numRetries = 0;
                entry->unk0x166 = 1;

                resultScreen->LinkScoreEx(entry, i, j);
                strcpy(entry->name, "--------");
                strcpy(entry->date, "--/--");
            }
        }
    }

    if (resultScreen->screenMode != RESULT_SCREEN_ONE_SHOT)
    {
        if (g_AnmManager->LoadSurface(0, "result/result.jpg") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }

        *(i32 *)((u8 *)resultScreen + 0x11440) = (i32)g_AnmManager->LoadAnm(0x15, "result00.anm");
        if (*(i32 *)((u8 *)resultScreen + 0x11440) == 0)
        {
            return ZUN_ERROR;
        }

        *(i32 *)((u8 *)resultScreen + 0x11444) = (i32)g_AnmManager->LoadAnm(0x16, "resulttext.anm");
        if (*(i32 *)((u8 *)resultScreen + 0x11444) == 0)
        {
            return ZUN_ERROR;
        }

        vm = resultScreen->vms;
        for (i = 0; i < 0x48; i++, vm++)
        {
            pos = Float3(0.0f, 0.0f, 0.0f);
            vm->pos = pos;
            pos2 = Float3(0.0f, 0.0f, 0.0f);
            vm->pos2 = pos2;
            ((AnmLoaded *)*(i32 *)((u8 *)resultScreen + 0x11440))->SetAndExecuteScriptIdx(vm, i);
        }

        ((AnmLoaded *)*(i32 *)((u8 *)resultScreen + 0x11440))
            ->InitializeAndSetSprite((AnmVm *)((u8 *)resultScreen + 0x1119c), 0x20);

        vm = resultScreen->scoreVms;
        for (i = 0; i < 0xe; i++, vm++)
        {
            ((AnmLoaded *)*(i32 *)0x17ce8f4)->InitializeAndSetSprite(vm, i + 0x15);
            pos = Float3(0.0f, 0.0f, 0.0f);
            vm->pos = pos;
            vm->prefix.type |= 0x1800;
            vm->fontWidth = 0xf;
            vm->fontHeight = 0xf;
        }

        vm = resultScreen->scoreVms;
        for (i = 0; i < 0xe; i++, vm++)
        {
            ((AnmLoaded *)*(i32 *)((u8 *)resultScreen + 0x11444))->InitializeAndSetSprite(vm, i + 2);
            pos = Float3(0.0f, 0.0f, 0.0f);
            vm->pos = pos;
            vm->prefix.type |= 0x1800;
            vm->fontWidth = 0xf;
            vm->fontHeight = 0xf;
        }
    }

    *(i32 *)((u8 *)resultScreen + 0x20) = 0;
    resultScreen->scoreData = (i32)ScoreDat::OpenScore("score.dat");

    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 0xc; j++)
        {
            ScoreDat::GetHighScore((ScoreDat *)resultScreen->scoreData, &resultScreen->scores[i][j], j, i, NULL);
        }
    }

    *(u32 *)&resultScreen->saveNameData[0] = LSNM_MAGIC;
    resultScreen->saveNameData[8] = LSNM_VERSION;
    *(u16 *)&resultScreen->saveNameData[6] = 0x18;
    *(u16 *)&resultScreen->saveNameData[4] = 0x18;
    strcpy(&resultScreen->saveNameData[0xc], "        ");
    resultScreen->nameEntryFlag = ScoreDat::ParseLSNM((ScoreDat *)resultScreen->scoreData, (Lsnm *)&resultScreen->saveNameData);

    if (resultScreen->screenMode != RESULT_SCREEN_RESULT && resultScreen->screenMode != RESULT_SCREEN_PRACTICE_RESULT
        && resultScreen->screenMode != RESULT_SCREEN_SPELL_PRACTICE_RESULT && resultScreen->screenMode != RESULT_SCREEN_ONE_SHOT)
    {
        ScoreDat::ParseCATK((ScoreDat *)resultScreen->scoreData, (Catk *)0x160f548);
        g_GameManager.IsPhantasmUnlocked();
        ScoreDat::ParseCLRD((ScoreDat *)resultScreen->scoreData, (Clrd *)0x164b998);
        ScoreDat::ParsePSCR((ScoreDat *)resultScreen->scoreData, (Pscr *)0x164bb6c);
    }

    if (resultScreen->screenMode == RESULT_SCREEN_PRACTICE_RESULT)
    {
        i32 character = (u8)(*(u8 *)0x164d0b1 + *(u8 *)0x164d0b2);
        i32 difficulty = *(i32 *)0x164d2cc;

        if (*(i32 *)(character * 0x178 + difficulty * 0x14 + 0x164bc2c + *(i32 *)0x160f538 * 4) <
            *(i32 *)((i32)resultScreen->scoreData + 8))
        {
            *(i32 *)(character * 0x178 + difficulty * 0x14 + 0x164bc2c + *(i32 *)0x160f538 * 4) =
                *(i32 *)((i32)resultScreen->scoreData + 8);
        }

        resultScreen->screenMode = RESULT_SCREEN_REPLAY_SAVE1;
        strcpy(resultScreen->nameBuffer, &resultScreen->saveNameData[0xc]);

        if (g_Supervisor.IsSlowModeEnabled() || g_Supervisor.IsClearBackBufferOnRefreshEnabled())
        {
            memcpy((void *)0x160f548, (const void *)0x162d770, 0x1e228);
        }
    }

    if (resultScreen->screenMode == RESULT_SCREEN_SPELL_PRACTICE_RESULT)
    {
        resultScreen->screenMode = RESULT_SCREEN_REPLAY_SAVE1;
        strcpy(resultScreen->nameBuffer, &resultScreen->saveNameData[0xc]);

        if (g_Supervisor.IsSlowModeEnabled() || g_Supervisor.IsClearBackBufferOnRefreshEnabled())
        {
            memcpy((void *)0x160f548, (const void *)0x162d770, 0x1e228);
        }
    }

    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 0xd; j++)
        {
            resultScreen->scoreCounts[i][j] = 0;
        }

        for (spellIdx = 0; spellIdx < *(i32 *)(i * 4 + 0x4c67e8); spellIdx++)
        {
            catk = (Catk *)(*(i32 *)(*(i32 *)(i * 4 + 0x4c67d0) + spellIdx * 4) * 0x22c + 0x160f548);
            if (catk->base.magic == CATK_MAGIC && catk->base.version == CATK_VERSION)
            {
                if (catk->inGameHistory.captures[j] != 0 || catk->spellPracticeHistory.captures[j] != 0)
                {
                    resultScreen->scoreCounts[i][j]++;
                }
            }
        }
    }

    resultScreen->savedSpellDifficultyCursor = 5;
    resultScreen->shotTypeCursor = 0xc;
    resultScreen->previousShotTypeCursor = 0xc;
    resultScreen->shotCursorMoved = 0;
    *(u16 *)((u8 *)resultScreen + 0x1110c) |= 0xffff;

    *(i32 *)0x164cf0c = *(i32 *)0x164cd74 + *(i32 *)0x164cdb8 + *(i32 *)0x164cdfc + *(i32 *)0x164ce40 + *(i32 *)0x164ce84;
    *(i32 *)0x164cf08 = *(i32 *)0x164cd70 + *(i32 *)0x164cdb4 + *(i32 *)0x164cdf8 + *(i32 *)0x164ce3c + *(i32 *)0x164ce80;

    if (resultScreen->screenMode == RESULT_SCREEN_ONE_SHOT)
    {
        if (g_Supervisor.IsSlowModeEnabled() || g_Supervisor.IsClearBackBufferOnRefreshEnabled())
        {
            ScoreDat::ParseCATK((ScoreDat *)resultScreen->scoreData, (Catk *)0x160f548);
        }

        resultScreen->DeletedCallback(resultScreen);
        return ZUN_ERROR;
    }

    resultScreen->savedDifficultyCursor = 1;
    resultScreen->savedCharacterCursor = 0;
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x459fd2
ZunResult ResultScreen::DeletedCallback(ResultScreen *resultScreen)
{
    if (resultScreen->scoreData != NULL)
    {
        resultScreen->WriteScore();
        ScoreDat::ReleaseScore((ScoreDat *)resultScreen->scoreData);
    }
    resultScreen->scoreData = NULL;

    for (i32 i = 0; i < 5; i++)
    {
        for (i32 j = 0; j < 12; j++)
        {
            resultScreen->FreeScore(i, j);
        }
    }

    g_AnmManager->ReleaseAnm(0x15);
    g_AnmManager->ReleaseAnm(0x16);
    g_AnmManager->ReplaceSurface(8, 0);

    g_Chain.Cut(resultScreen->drawChain);
    resultScreen->drawChain = NULL;

    g_ZunMemory.RemoveFromRegistry(resultScreen);

    delete resultScreen;
    resultScreen = NULL;

    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x45a0f4
i32 ResultScreen::MoveCursor(i32 length)
{
    if (WAS_PRESSED_SCROLLING(0x10))
    {
        this->cursor--;
        if (this->cursor < 0)
        {
            this->cursor += length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }

    if (WAS_PRESSED_SCROLLING(0x20))
    {
        this->cursor++;
        if (this->cursor >= length)
        {
            this->cursor -= length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }

    return 0;
}

// FUNCTION: th08 0x45a1f3
i32 ResultScreen::MoveShotTypeCursor(i32 length)
{
    if (WAS_PRESSED_SCROLLING(0x10))
    {
        this->shotTypeCursor--;
        if (this->shotTypeCursor < 0)
        {
            this->shotTypeCursor += length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }

    if (WAS_PRESSED_SCROLLING(0x20))
    {
        this->shotTypeCursor++;
        if (this->shotTypeCursor >= length)
        {
            this->shotTypeCursor -= length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }

    return 0;
}

// FUNCTION: th08 0x45a2f2
i32 ResultScreen::MoveCursorHorizontally(i32 length)
{
    if (WAS_PRESSED_SCROLLING(0x40))
    {
        this->cursor--;
        if (this->cursor < 0)
        {
            this->cursor += length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }

    if (WAS_PRESSED_SCROLLING(0x80))
    {
        this->cursor++;
        if (this->cursor >= length)
        {
            this->cursor -= length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }

    return 0;
}

} /* namespace th08 */
