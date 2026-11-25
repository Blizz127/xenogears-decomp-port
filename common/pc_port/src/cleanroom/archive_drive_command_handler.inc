static void ArchiveDriveRecoverCommand(u_int error) {
    g_ArchiveCdDriveError = error;
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
    PcPort_ArchiveDriveCommand(CdlNop, NULL);
}

static void ArchiveDriveRestartRead(u_int error) {
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
    ++D_8005A488;
    ++D_8005A494;
    if (!PcPort_ArchiveRestoreReadyCallback()) {
        g_ArchiveCdDriveError = error;
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
        PcPort_ArchiveRefuseRecovery();
        return;
    }
    PcPort_ArchiveDriveCommand(CdlSetloc, (u_char *)&g_ArchiveCdCurLocation);
}

static void ArchiveDriveResumeError(void) {
    u_char command;
    u_char *parameter = NULL;

    switch (g_ArchiveCdDriveError) {
    case ARCHIVE_CD_DRIVE_ERR_SEEK:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SEEK;
        command = CdlSetloc;
        parameter = (u_char *)&g_ArchiveCdCurLocation;
        break;
    case ARCHIVE_CD_DRIVE_ERR_GENERIC:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
        command = CdlPause;
        break;
    case ARCHIVE_CD_DRIVE_ERR_READ:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_DATA_SECTOR;
        command = CdlPause;
        break;
    case ARCHIVE_CD_DRIVE_ERR_HALT:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_HALT;
        command = CdlStop;
        break;
    case ARCHIVE_CD_DRIVE_ERR_ADPCM:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL;
        command = CdlSetmode;
        parameter = D_80059F18;
        break;
    case ARCHIVE_CD_DRIVE_ERR_RESET_MODE:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_RESET_MODE;
        command = CdlSetmode;
        parameter = D_80059F18;
        break;
    default:
        return;
    }
    PcPort_ArchiveDriveCommand(command, parameter);
}

void ArchiveCdDriveCommandHandler(u_char status, u_char *pResult) {
    if (!PcPort_ArchiveDriveAcceptNotification(status, pResult)) {
        return;
    }

    u_int state = g_ArchiveCdDriveState;
    if (status != CdlComplete) {
        u_int error;
        switch (state) {
        case ARCHIVE_CD_DRIVE_READ_SECTOR:
        case ARCHIVE_CD_DRIVE_READ_DONE:
            ++D_8005A490;
            PcPort_ArchiveSaveReadyCallback();
            error = ARCHIVE_CD_DRIVE_ERR_READ;
            break;
        case ARCHIVE_CD_DRIVE_SEEK:
        case ARCHIVE_CD_DRIVE_SEEK_DONE:
            error = ARCHIVE_CD_DRIVE_ERR_SEEK;
            break;
        case ARCHIVE_CD_DRIVE_DONE:
            error = ARCHIVE_CD_DRIVE_ERR_GENERIC;
            break;
        case ARCHIVE_CD_DRIVE_READ_DATA_SECTOR:
        case ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR:
            ++D_8005A498;
            error = state == ARCHIVE_CD_DRIVE_READ_DATA_SECTOR ?
                ARCHIVE_CD_DRIVE_ERR_READ : ARCHIVE_CD_DRIVE_ERR_ADPCM;
            break;
        case ARCHIVE_CD_DRIVE_HALT:
        case ARCHIVE_CD_DRIVE_RESET_MODE:
            ++D_8005A4B4;
            error = state == ARCHIVE_CD_DRIVE_HALT ?
                ARCHIVE_CD_DRIVE_ERR_HALT : ARCHIVE_CD_DRIVE_ERR_RESET_MODE;
            break;
        case ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL:
            error = ARCHIVE_CD_DRIVE_ERR_ADPCM;
            break;
        case ARCHIVE_CD_DRIVE_ERROR:
        case ARCHIVE_CD_DRIVE_HANDLE_ERROR:
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            PcPort_ArchiveDriveCommand(CdlNop, NULL);
            return;
        default:
            return;
        }
        ArchiveDriveRecoverCommand(error);
        return;
    }

    switch (state) {
    case ARCHIVE_CD_DRIVE_READ_SECTOR:
        ++D_8005A48C;
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_DONE;
        PcPort_ArchiveDriveCommand(CdlReadN, NULL);
        break;
    case ARCHIVE_CD_DRIVE_READ_DONE:
        ++D_8005A48C;
        /* fall through */
    case ARCHIVE_CD_DRIVE_SEEK_DONE:
    case ARCHIVE_CD_DRIVE_DONE:
        PcPort_ArchiveDriveClearCallback();
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
        break;
    case ARCHIVE_CD_DRIVE_SEEK:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SEEK_DONE;
        PcPort_ArchiveDriveCommand(CdlSeekL, NULL);
        break;
    case ARCHIVE_CD_DRIVE_READ_DATA_SECTOR:
        ArchiveDriveRestartRead(ARCHIVE_CD_DRIVE_ERR_READ);
        break;
    case ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR:
        ArchiveDriveRestartRead(ARCHIVE_CD_DRIVE_ERR_ADPCM);
        break;
    case ARCHIVE_CD_DRIVE_HALT:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_DATA_SECTOR;
        ++D_8005A4A8;
        PcPort_ArchiveDriveCommand(CdlPause, NULL);
        break;
    case ARCHIVE_CD_DRIVE_RESET_MODE:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
        PcPort_ArchiveDriveCommand(CdlPause, NULL);
        break;
    case ARCHIVE_CD_DRIVE_ERROR:
        if (pResult[0] & 0x10) {
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            PcPort_ArchiveDriveCommand(CdlNop, NULL);
        } else {
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_HANDLE_ERROR;
            PcPort_ArchiveDriveCommand(CdlGetTN, NULL);
        }
        break;
    case ARCHIVE_CD_DRIVE_HANDLE_ERROR:
        ArchiveDriveResumeError();
        break;
    case ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL:
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR;
        D_80059F14[1] = (u_char)D_8004FE38;
        D_80059F14[0] = 1;
        PcPort_ArchiveDriveCommand(CdlSetfilter, (u_char *)D_80059F14);
        break;
    }
}
