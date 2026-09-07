#pragma once
// Stable Score Context command names corresponding to native Tracker enums.
static const char *SemanticName(VolumeCommand command)
{
	switch(command)
	{
	case VOLCMD_NONE: return "none";
	case VOLCMD_VOLUME: return "volume";
	case VOLCMD_PANNING: return "panning";
	case VOLCMD_VOLSLIDEUP: return "volslideup";
	case VOLCMD_VOLSLIDEDOWN: return "volslidedown";
	case VOLCMD_FINEVOLUP: return "finevolup";
	case VOLCMD_FINEVOLDOWN: return "finevoldown";
	case VOLCMD_VIBRATOSPEED: return "vibratospeed";
	case VOLCMD_VIBRATODEPTH: return "vibratodepth";
	case VOLCMD_PANSLIDELEFT: return "panslideleft";
	case VOLCMD_PANSLIDERIGHT: return "panslideright";
	case VOLCMD_TONEPORTAMENTO: return "toneportamento";
	case VOLCMD_PORTAUP: return "portaup";
	case VOLCMD_PORTADOWN: return "portadown";
	case VOLCMD_PLAYCONTROL: return "playcontrol";
	case VOLCMD_OFFSET: return "offset";
	default: return "unknown";
	}
}
static const char *SemanticName(EffectCommand command)
{
	switch(command)
	{
	case CMD_NONE: return "none";
	case CMD_ARPEGGIO: return "arpeggio";
	case CMD_PORTAMENTOUP: return "portamentoup";
	case CMD_PORTAMENTODOWN: return "portamentodown";
	case CMD_TONEPORTAMENTO: return "toneportamento";
	case CMD_VIBRATO: return "vibrato";
	case CMD_TONEPORTAVOL: return "toneportavol";
	case CMD_VIBRATOVOL: return "vibratovol";
	case CMD_TREMOLO: return "tremolo";
	case CMD_PANNING8: return "panning8";
	case CMD_OFFSET: return "offset";
	case CMD_VOLUMESLIDE: return "volumeslide";
	case CMD_POSITIONJUMP: return "positionjump";
	case CMD_VOLUME: return "volume";
	case CMD_PATTERNBREAK: return "patternbreak";
	case CMD_RETRIG: return "retrig";
	case CMD_SPEED: return "speed";
	case CMD_TEMPO: return "tempo";
	case CMD_TREMOR: return "tremor";
	case CMD_MODCMDEX: return "modcmdex";
	case CMD_S3MCMDEX: return "s3mcmdex";
	case CMD_CHANNELVOLUME: return "channelvolume";
	case CMD_CHANNELVOLSLIDE: return "channelvolslide";
	case CMD_GLOBALVOLUME: return "globalvolume";
	case CMD_GLOBALVOLSLIDE: return "globalvolslide";
	case CMD_KEYOFF: return "keyoff";
	case CMD_FINEVIBRATO: return "finevibrato";
	case CMD_PANBRELLO: return "panbrello";
	case CMD_XFINEPORTAUPDOWN: return "xfineportaupdown";
	case CMD_PANNINGSLIDE: return "panningslide";
	case CMD_SETENVPOSITION: return "setenvposition";
	case CMD_MIDI: return "midi";
	case CMD_SMOOTHMIDI: return "smoothmidi";
	case CMD_DELAYCUT: return "delaycut";
	case CMD_XPARAM: return "xparam";
	case CMD_FINETUNE: return "finetune";
	case CMD_FINETUNE_SMOOTH: return "finetune_smooth";
	case CMD_DUMMY: return "dummy";
	case CMD_NOTESLIDEUP: return "noteslideup";
	case CMD_NOTESLIDEDOWN: return "noteslidedown";
	case CMD_NOTESLIDEUPRETRIG: return "noteslideupretrig";
	case CMD_NOTESLIDEDOWNRETRIG: return "noteslidedownretrig";
	case CMD_REVERSEOFFSET: return "reverseoffset";
	case CMD_DBMECHO: return "dbmecho";
	case CMD_OFFSETPERCENTAGE: return "offsetpercentage";
	case CMD_DIGIREVERSESAMPLE: return "digireversesample";
	case CMD_VOLUME8: return "volume8";
	case CMD_HMN_MEGA_ARP: return "hmn_mega_arp";
	case CMD_MED_SYNTH_JUMP: return "med_synth_jump";
	case CMD_AUTO_VOLUMESLIDE: return "auto_volumeslide";
	case CMD_AUTO_PORTAUP: return "auto_portaup";
	case CMD_AUTO_PORTADOWN: return "auto_portadown";
	case CMD_AUTO_PORTAUP_FINE: return "auto_portaup_fine";
	case CMD_AUTO_PORTADOWN_FINE: return "auto_portadown_fine";
	case CMD_AUTO_PORTAMENTO_FC: return "auto_portamento_fc";
	case CMD_TONEPORTA_DURATION: return "toneporta_duration";
	case CMD_VOLUMEDOWN_DURATION: return "volumedown_duration";
	case CMD_VOLUMEDOWN_ETX: return "volumedown_etx";
	default: return "unknown";
	}
}
