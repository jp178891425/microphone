/**
 **************************************************************************************
 * @file    ctrlvars.h
 * @brief   Control Variables Definition
 * 
 * @author  Aissen Li
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
#ifndef __CTRLVARS_H__
#define __CTRLVARS_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>
#include "audio_effect.h"
#include "audio_effect_library.h"
#include "app_config.h"
#include "bt_config.h"
#include "audio_core_api.h"
#include "blue_aec.h"
#include "vocalcut.h"
#ifdef CFG_FUNC_TSM_EN
       #include "tsm.h"
#endif
#include "dra_app.h"
#define  CTRL_VARS_SYNC_WORD            (0x43564347)
#define  EFFECT_LIST_LEN                80

#define  MIN_VOLUME                     (0)
#define  MAX_VOLUME                     (63)
#define  VOLUME_COUNT                   (MAX_VOLUME - MIN_VOLUME + 1)
#define  MIN_BASS_TREB_GAIN             (0)
#define  MAX_BASS_TREB_GAIN             (15)
#define  MIN_MIC_DIG_STEP               (0)
#define  MAX_MIC_DIG_STEP               (32)
#define  MIN_MIC_REVB_STEP              (0)
#define  MAX_MIC_REVB_STEP              (32)
#define  MIN_MUSIC_DIG_STEP             (0)
#define  MAX_MUSIC_DIG_STEP             (32)
#define  BASS_TREB_GAIN_STEP            (1)
#define  MIN_AUTOTUNE_STEP              (0)
#define  MAX_AUTOTUNE_STEP              (11)

#define AEC_SIZE		    			(sizeof(BlueAECContext))
#define PCM_DELAY_SIZE					(sizeof(PCMDelay))
#define EXCITER_SIZE					(sizeof(ExciterContext))
#define EXPANDER_SIZE					(sizeof(ExpanderContext))
#define AUTOTUNE_SIZE					(sizeof(AutoTuneContext16))
#define DRC_SIZE						(sizeof(DRCContext))
#define ECHO_SIZE						(sizeof(EchoContext))
#define ECHO_S_SIZE 					(22100)
#define FREQSHIFTER_SIZE				(sizeof(FreqShifterContext))
#define FREQSHIFTER_FINE_SIZE			(sizeof(FreqShifterFineContext))
#define HOWLING_SIZE					(sizeof(HowlingContext))
#define HOWLING_FINE_SIZE		        (sizeof(HowlingFineContext))
#define SILENCE_SIZE					(sizeof(SilenceDetectorContext))
#define PS_SIZE							(sizeof(PSContext))
#define PS_PRO_SIZE		    			(sizeof(PitchShifterProContext))
#define REVERB_SIZE						(sizeof(ReverbContext))
#define REVERB_PLATE16_SIZE				(sizeof(ReverbPlateContext16))
#define REVERB_PLATE24_SIZE				(sizeof(ReverbPlateContext24))
#define REVERB_PRO_SIZE     			MAX_REVERB_PRO_CONTEXT_SIZE_44100
#define REVERB_PRO_32K_SIZE 			MAX_REVERB_PRO_CONTEXT_SIZE_32000
#define VOICECHANGER_SIZE				(sizeof(VoiceChangerContext))
#define VOICECHANGER_PRO_SIZE			(sizeof(VoiceChangerProContext))
#define EQ_SIZE							(sizeof(EQContext))
#define BIQUAD_SIZE						(sizeof(BiquadContext))
#define VOCALCUT_SIZE	    			(sizeof(VocalCutContext))
#define VOCALREMOVE_SIZE				(sizeof(VocalRemoverContext))
#define THREED_SIZE         			(sizeof(ThreeDContext))
#define THREED_PLUS_SIZE         		(sizeof(ThreeDPlusContext))
#define VB_SIZE             			(sizeof(VBContext))
#define VB_TD_SIZE             			(sizeof(VBTDContext))
#define VBCLASSIC_SIZE      			(sizeof(VBClassicContext))
#define CHORUS_SIZE         			(sizeof(ChorusContext))+4
#define CHORUS2_SIZE         			(sizeof(Chorus2Context))+4
#define STEREO_WIDEN_SIZE         		(sizeof(StereoWidenerContext))
#define PINGPONG_SIZE         		    (sizeof(PingPongContext))
#define AUTOWAH_SIZE         		    (sizeof(AutoWahContext))
#define FLANGER_SIZE                    (sizeof(FlangerContext))
#define OVERDRIVE_SIZE                  (sizeof(OverdriveContext))
#define DISTORTION_SIZE                 (sizeof(DistortionExpContext))
#define PITCH_DECTOR_SIZE               (sizeof(PitchDetectorContext))
#define BLUE_NS_128_SIZE                (sizeof(BlueNSContext128))
#define BLUE_NS_256_SIZE                (sizeof(BlueNSContext256))
#define BLUE_NS_512_SIZE                (sizeof(BlueNSContext512))
#define PSDET_SIZE			            (sizeof(PitchDetectorContext))
#define EQ_DRC_STRUCT_SIZE              (sizeof(EQDRCContext))
#define FADER_SIZE                      (sizeof(FaderContext))
#define OVERDRIVER_POLY_SIZE            (sizeof(OverdrivePolyContext))
#define DISTORTION_DS1_SIZE             (sizeof(DistortionDS1Context))
#define COMPANDER_SIZE                  (sizeof(CompanderContext))
#define LOW_LEVEL_COMPRESSOR_SIZE       (sizeof(LowLevelCompressorContext))
#define HOWLINGGUARD_SIZE               (sizeof(HowlingGuardContext))
#define PHASE_SHIFTER_SIZE              (sizeof(PhaseShifterContext))
#define DRA_POST_SIZE                   (sizeof(DraPostProcessingContext))
#define TSM_SIZE                        (sizeof(TSMContext))
#define TSM_FIFO_SIZE  					TSM_MAX_W_SIZE * 8

//-------only for print audio effect ram malloc---------------//
enum
{
    EFF_AUTO_TUNE=0,
    EFF_DC_BLOCK,
    EFF_DRC,
    EFF_ECHO,
    EFF_EQ,
    EFF_EXPAND,
    EFF_FREQ_SHIFTER,
    EFF_HOWLING_DETOR,
    EFF_NOISE_GATE,
    EFF_PITCH,
    EFF_REVERB,
    EFF_SILENCE_DETOR,
    EFF_THREED,
    EFF_VB,
    EFF_VOICE_CHANGE,
    EFF_GAIN,
    EFF_VOCALCUT,
    EFF_PLATE_REVERB,
    EFF_REVERB_PRO,
    EFF_VOICE_CHANGE_PRO,
    EFF_PHASE_CTL=20,
    EFF_VOCALREMOVE,
    EFF_PITCH_PRO,
    EFF_VBCLASSIC,
    EFF_PCM_DELAY,
    EFF_EXCITER,
    EFF_AEC,
    EFF_CHORUS,
	EFF_STEREO_WIDEN,
	EFF_PINGPONG,
	EFF_AUTOWAH,
	EFF_Chorus2 = 101,//user define ,effect
	EFF_HowlingGuard,
	EFF_PhaseShifter,
	EFF_DraPost1,
	EFF_DraPost2,
	EFF_DraPost3,
	EFF_DraPost4,
	EFF_DraPost5,
	EFF_DCBlocker,
	EFF_VBSurround,
	EFF_Butterworth,
	EFF_DynamicEQ,
	EFF_AEC_USER,
	EFF_NS_LEVEL,
	EFF_LR_BALANCER,
	EFF_HOWLING_SPECIFIED,
	EFF_HOWLING_SPECIFIED_1,
	EFF_VAD,
	EFF_DRC_LEGACY,
};

#define  CFG_I2S0_OUT_EN 1
#define  CFG_I2S0_IN_EN  1
#define  CFG_I2S1_OUT_EN 1
#define  CFG_I2S1_IN_EN  1
#define  CFG_USB_OUT_EN              1
#define  CFG_USB_OUT_STEREO_EN       1
#define  CFG_SUPPORT_USB_VOLUME_SET  0
	
typedef enum _EFFECT_MODE
{
    EFFECT_MODE_NORMAL=10,
	EFFECT_MODE_ECHO, 
	EFFECT_MODE_REVERB,
	EFFECT_MODE_ECHO_REVERB,
    EFFECT_MODE_PITCH_SHIFTER,    
    EFFECT_MODE_VOICE_CHANGER,   
    EFFECT_MODE_AUTO_TUNE, 
    EFFECT_MODE_BOY,
    EFFECT_MODE_GIRL,
    EFFECT_MODE_KTV,
    EFFECT_MODE_BaoYin,
    
    EFFECT_MODE_HunXiang,    
    EFFECT_MODE_DianYin,
    EFFECT_MODE_MoYin,
    EFFECT_MODE_HanMai,
    EFFECT_MODE_NanBianNv,
    EFFECT_MODE_NvBianNan,
    EFFECT_MODE_WaWaYin,   
    #ifdef BT_TWS_SUPPORT
	EFFECT_MODE_HunXiang_Slave,    
    EFFECT_MODE_DianYin_Slave,
    EFFECT_MODE_MoYin_Slave,
    EFFECT_MODE_HanMai_Slave,
    EFFECT_MODE_NanBianNv_Slave,
    EFFECT_MODE_NvBianNan_Slave,
    EFFECT_MODE_WaWaYin_Slave,
	EFFECT_MODE_NORMAL_SLAVE,
	#endif
	EFFECT_MODE_YuanSheng,
    EFFECT_MODE_HFP_AEC,
    EFFECT_MODE_USB_AEC,
    //User can add other effect mode
} EFFECT_MODE;

#ifdef BT_TWS_SUPPORT
#ifdef CFG_FUNC_MIC_KARAOKE_EN
#define  EFFECT_MODE_SLAVE_INDEX   (EFFECT_MODE_WaWaYin - EFFECT_MODE_HunXiang+1)
#else
#define  EFFECT_MODE_SLAVE_INDEX   (EFFECT_MODE_WaWaYin - EFFECT_MODE_NORMAL+8)
#endif
#endif
//--------------------------------------//
enum
{
    boolType = 0,
	enumType,
	continueType,
	dispType,

};

typedef  void (*EffectFunc)(uint8_t , uint32_t , uint8_t *, uint32_t);

typedef struct _ParaNumbers_
{
	int32_t     paramete_totals;
}ParaNumbers;

typedef struct _TypeBool_
{
	//int32_t     index;
	int32_t     data_type;
	int32_t     defualt;
	char        *name;
}TypeBool;

typedef struct _TypeEnum_
{
	//int32_t     index;
	int32_t     data_type;
	char        *enum_name;
	int32_t     defualt;
	char        *name;
}TypeEnum;

typedef struct _TypeContinue_
{
	//int32_t     index;
	int32_t     data_type;
	int32_t     min;
	int32_t     max;
	int32_t     step;
	int32_t     ratio;
	int32_t     fraction;
	int32_t     defualt;
	char        *name;
	char        *unit;
}TypeContinue;

typedef struct _TypeDisp_
{
	//int32_t     index;
	int32_t     data_type;
	int32_t     min;
	int32_t     max;
	int32_t     ratio;
	int32_t     fraction;
	int32_t     defualt;
	char        *name;
	char        *unit;
}TypeDisp;

#define ParameterTotasSize   sizeof(ParaNumbers)/sizeof(int32_t)
#define LogicSize            sizeof(TypeBool)/sizeof(int32_t)
#define ContinueSize         sizeof(TypeContinue)/sizeof(int32_t)
#define EnumSize             sizeof(TypeEnum)/sizeof(int32_t)
#define DispSize             sizeof(TypeEnum)/sizeof(int32_t)


//---------------------------------------------//
typedef enum _REMIND_TYPE
{
    REMIND_TYPE_KEY,
	REMIND_TYPE_BACKGROUND, 

} REMIND_TYPE;

typedef enum _EQ_MODE
{
    EQ_MODE_FLAT,
	EQ_MODE_CLASSIC,	
	EQ_MODE_POP,
	EQ_MODE_ROCK,
	EQ_MODE_JAZZ,
	EQ_MODE_VOCAL_BOOST,
} EQ_MODE;

typedef struct __MixInput
{
	int16_t     **music_in;
#ifdef CFG_FUNC_LINE_MIX_MODE
	int16_t      line_mix_en;
	int16_t     **line_in;
#endif
#ifdef CFG_RES_AUDIO_I2S0IN_EN
	int16_t      i2s0_mix_en;
	int16_t     **i2s0_in;
#endif
#ifdef CFG_RES_AUDIO_I2S1IN_EN
	int16_t      i2s1_mix_en;
	int16_t     **i2s1_in;
#endif
#ifdef CFG_FUNC_USB_MIX_MODE
	int16_t      usb_mix_en;
	int16_t     **usb_in;
#endif
#ifdef CFG_FUNC_REMIND_MIX_MODE
	int16_t     remind_mix_en;
	int16_t     **remind_mix_in;
#endif
#ifdef CFG_FUNC_SPDIF_MIX_MODE
	int16_t      spdif_mix_en;
	int16_t     **spdif_in;
#endif
} MixInputUnit;

#define ANA_INPUT_CH_NONE    0
#define ANA_INPUT_CH_LINEIN1 1
#define ANA_INPUT_CH_LINEIN2 2
#define ANA_INPUT_CH_LINEIN3 3
#define ANA_INPUT_CH_LINEIN4 4
#define ANA_INPUT_CH_LINEIN5 5


typedef struct
{
	uint16_t       eff_mode;
	const uint8_t *EffectParamas;
	uint16_t	  len;
	char          *name;
	  
}AUDIO_EFF_PARAMAS;

//--------audio effect unit define-----------------------------------//
#ifdef CFG_FUNC_TSM_EN
typedef struct __TSMUnit
{
	TSMContext  *ct;
	uint8_t      tsm_en;
	int32_t      speed_ratio;
	uint16_t     in_size;
	uint16_t     out_size;
	int16_t      *in_buf;
	int16_t      *out_buf;

} TSMUnit;
#endif//

typedef struct __PcmDelayUnit
{
	PCMDelay           *ct;
	uint16_t  		   enable;
	uint16_t           delay;
	uint16_t           max_delay;
	uint16_t           high_quality;
	//-----------------------------------//
	int32_t    		   delay_samples;
	int32_t 		   max_delay_samples;
	uint8_t            *s_buf;
	uint8_t            channel;
	uint8_t            bit_width;
} PcmDelayUnit;

typedef struct __ExciterUnit
{
	ExciterContext      *ct;
	int16_t  			enable;
	int16_t 			f_cut;
	int16_t 			dry;
	int16_t 			wet;
	uint8_t             channel;

} ExciterUnit;

typedef struct __AutoTuneUnit
{
	void             *ct;
	int16_t 		 enable;
	int16_t 		 key;
	int16_t  		 snap_mode;
	int16_t          pitch_accuracy;
	uint8_t          channel;
	uint8_t          bit_width;
} AutoTuneUnit;

typedef struct __DRCUnit
{
	DRCContext 		*ct;
	int16_t 		 enable;
	int16_t 		 mode;
	int16_t          cf_type;
	int16_t 		 q_l;
	int16_t 		 q_h;
	int32_t 		 fc[2];
	int32_t  		 threshold[4];
	int32_t  		 ratio[4];
	int32_t  		 attack[4];
	int32_t  		 release[4];
	int32_t  		 pregain[4];
	uint8_t          channel;
} DRCUnit;

typedef struct __EchoUnit
{
	EchoContext 	*ct;
	uint16_t 		 enable;
	uint16_t  		 fc;
	uint16_t  		 attenuation;
	uint16_t  		 delay;
	uint16_t  		 reserve;
	uint16_t  		 max_delay;
	uint16_t         high_quality;//0 =normal echo, 1= high_quality echo
	uint16_t         dry;
	uint16_t         wet;
	//-----------------------------//
	int32_t  		 delay_samples;
	int32_t 		 max_delay_samples;
	uint16_t         delay_bakup;
	uint8_t			 *s_buf;
	uint8_t          channel;
	uint8_t          bit_width;

} EchoUnit;

typedef struct __ExpanderUnit
{
	ExpanderContext *ct;
	int16_t  		 enable;
	int16_t   		 threshold;
	int16_t   		 ratio;
	int16_t   		 attack_time;
	int16_t   		 release_time;
	uint8_t          channel;

} ExpanderUnit;


typedef struct __FreqShifterUnit
{
	FreqShifterContext *ct;
	int16_t  			enable;
	int16_t    			deltaf;
	uint8_t             channel;

} FreqShifterUnit;

typedef struct __FreqShifterFineUnit
{
	FreqShifterFineContext *ct;
	int16_t  			enable;
	int16_t    			deltaf;

} FreqShifterFineUnit;


typedef struct __HowlingDectorUnit
{
	HowlingContext 		*ct;
	int16_t        	    enable;
	int16_t 		   suppression_mode;
	uint8_t              channel;

} HowlingDectorUnit;

typedef struct __HowlingDectorFineUnit
{
	HowlingFineContext 	*ct;
	int16_t        	     enable;
	int16_t 			 q_min;
	int16_t   		     q_max;
} HowlingDectorFineUnit;

typedef struct __SilenceDetectorUnit
{
	SilenceDetectorContext *ct;
	int16_t        	       enable;
	int16_t        	       level;
	uint8_t                channel;

} SilenceDetectorUnit;

typedef struct __VocalCutUnit
{
	VocalCutContext *ct;
	int16_t 	   enable;
	int16_t 	   wetdrymix;
	int16_t 	   vocal_cut_en;
	uint8_t        channel;

} VocalCutUnit;

typedef struct __VocalRemoveUnit
{
	void                 *ct;
	int16_t 	          enable;
	int16_t 	          lower_freq;
	int16_t 	          higher_freq;
	int16_t 	          step_size;
	int16_t               vocal_remove_en;
	uint8_t        		  channel;
	uint8_t               bit_width;
} VocalRemoveUnit;

typedef struct __ChorusUnit
{
	ChorusContext        *ct;
	int16_t 			 enable;
	int16_t 			 delay_length;
	int16_t 		     mod_depth;
	int16_t   	 		 mod_rate;
	int16_t   	 		 feedback;
	int16_t  	 		 dry;
	int16_t   	 		 wet;
} ChorusUnit;



typedef struct __Chorus2Describe
{
	ParaNumbers          numbers;//totals
	TypeContinue         P1;//delay_length
	TypeContinue         P2;//dry;
	TypeContinue         P3;//wet_1;
	TypeContinue         P4;//wet_2;
	TypeContinue         P5;//mod1_depth;
	TypeContinue         P6;//mod1_rate;
	TypeContinue         P7;//mod2_depth;
	TypeContinue         P8;//mod2_rate;
}Chorus2Describe;

typedef struct __Chorus2Unit
{
	void                 *ct;
	int16_t 			 enable;
	int16_t 			 delay_length;
	int16_t  	 		 dry;
	int16_t   	 		 wet_1;
	int16_t   	 		 wet_2;
	int16_t 		     mod1_depth;
	int16_t 		     mod1_rate;
	int16_t 		     mod2_depth;
	int16_t 		     mod2_rate;
	int16_t              bit_width;
} Chorus2Unit;

typedef struct __PitchShifterUnit
{
	void      			*ct;
	int16_t 			 enable;
	int16_t  			 semitone_steps;
	uint8_t              channel;
	uint8_t              bit_width;
} PitchShifterUnit;
#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
typedef struct __PitchShifterProUnit
{
	PitchShifterProContext 			*ct;
	int16_t 			 enable;
	int16_t  			 semitone_steps;
	uint8_t              channel;

} PitchShifterProUnit;
#endif
typedef struct __ReverbUnit
{
	void        	 *ct;
	int16_t 		 enable;
	int16_t 		 dry_scale;
	int16_t 		 wet_scale;
	int16_t 		 width_scale;
	int16_t 		 roomsize_scale;
	int16_t 		 damping_scale;
	int16_t          mono;
	uint8_t          channel;
	uint8_t          bit_width;
} ReverbUnit;

typedef struct __ReverbPlateUnit
{
	void                 *ct;
	int16_t 		     enable;
	int16_t              highcut_freq;
	int16_t              modulation_en;
	int16_t              predelay;
	int16_t              diffusion;
	int16_t              decay;
	int16_t              damping;
	int16_t              wetdrymix;
	uint8_t              channel;
	uint8_t              bit_width;//16,24

} ReverbPlateUnit;

#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
typedef struct __ReverbProUnit
{
	uint8_t *ct; 
	uint32_t enable;	
	uint32_t enable_bak;
	int32_t dry;
	int32_t wet;
	int32_t erwet;
	int32_t erfactor;
	int32_t erwidth;
	int32_t ertolate;
	int32_t rt60;
	int32_t delay;
	int32_t width;
	int32_t wander;
	int32_t spin;
	int32_t inputlpf;
	int32_t damplpf;
	int32_t basslpf;
	int32_t bassb;
	int32_t outputlpf;
	int32_t sample_rate;
	int16_t bit_width;
	uint8_t channel;
} ReverbProUnit;
#endif
#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
typedef struct __ThreeDUnit
{
	ThreeDContext *ct;
	int16_t 	   enable;
	int16_t  	   intensity;
	int16_t 	   three_d_en;
	uint8_t        channel;
} ThreeDUnit;

#endif
#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
typedef struct __ThreeDPlusUnit
{
	ThreeDPlusContext *ct;
	int16_t 	   enable;
	int16_t  	   intensity;
	int16_t 	   three_d_en;
	uint8_t        channel;

} ThreeDPlusUnit;
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
typedef struct __VBUnit
{
	void   	      *ct;
	int16_t 	  enable;
	int16_t 	  f_cut;
	int16_t 	  intensity;
	int16_t 	  enhanced;
	int16_t 	  vb_en;
	int16_t       vb_en_bak;
	uint8_t       channel;
	uint8_t       vb_type;//0=normal, 1=td

} VBUnit;
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
typedef struct __VBClassicUnit
{
	VBClassicContext 	 *ct;
	int16_t 	         enable;
	int16_t 	         f_cut;
	int16_t 	         intensity;
	int16_t 	         vb_en;
	int16_t              vb_en_bak;
	uint8_t              channel;

} VBClassicUnit;
#endif
typedef struct __StereoWindenUnit
{
	StereoWidenerContext *ct;
	int16_t 			 enable;
	int16_t 			 shaping;
	uint8_t              channel;
} StereoWindenUnit;

typedef struct __VoiceChangerUnit
{
	VoiceChangerContext *ct;
	int16_t 			 enable;
	int16_t 			 pitch_ratio;
	int16_t 		     formant_ratio;
	uint8_t              channel;

} VoiceChangerUnit;

#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
typedef struct __VoiceChangerProUnit
{
	VoiceChangerProContext *ct;
	int16_t 			 enable;
	int16_t 			 enable_bak;
	int16_t 			 pitch_ratio;
	int16_t 		     formant_ratio;
	uint8_t              channel;
} VoiceChangerProUnit;

#endif
typedef struct __AecUnit
{
	BlueAECContext *ct;
	int16_t 			 enable;
	int16_t 			 es_level;
} AecUnit;

typedef struct __FilterParams
{
	int16_t 	enable;
	int16_t		type;           /**< filter type, @see EQ_FILTER_TYPE_SET                               */
	uint16_t	f0;             /**< center frequency (peak) or mid-point frequency (shelf)             */
	int16_t		Q;              /**< quality factor (peak) or slope (shelf), format: Q6.10              */
	int16_t		gain;           /**< Gain in dB, format: Q8.8 */
} FilterParams;

typedef struct __EQUnit
{
	EQContext 		*ct;
	uint16_t         enable;
	int16_t 		 pregain;
	uint16_t         calculation_type;
	FilterParams	 eq_params[10];
	EQFilterParams  *filter_params;
	uint16_t   		 param_cnt;
	uint16_t 		 filter_count;
	uint8_t          channel;

} EQUnit;

typedef struct __BiquadUnit
{
	BiquadContext 	 *ct;
	int16_t          enable;
	int16_t 		 reserve1;
	int16_t          use_float;//15~8bits: use float,	7~0bits: output saturation
	int16_t 		 reserve2;
	FilterParams     eq_params[1];
	EQFilterParams  *filter_params;
	uint8_t          channel;
} BiquadUnit;


typedef struct __PingPongUnit
{
	PingPongContext  *ct;
	uint16_t          enable;
	uint16_t          attenuation;
	uint16_t          delay;
	uint16_t          high_quality;
	uint16_t          wetdrymix;
	uint16_t  		  max_delay;
	//-----------------------------//
	int32_t           delay_samples;
	int32_t 		  max_delay_samples;
	uint8_t           *s;
	uint8_t           bit_width;
} PingPongUnit;

typedef struct __AutoWahUnit
{
	AutoWahContext  *ct;
	uint16_t         enable;
	uint16_t         modulation_rate;
	uint16_t         min_frequency;
	uint16_t         max_frequency;
	uint16_t         depth;
	uint16_t 		 dry;
	uint16_t 		 wet;
} AutoWahUnit;

typedef struct __PhaseControlUnit
{
	int16_t      enable;
	int16_t      phase_difference;
	uint8_t      channel;
} PhaseControlUnit;

typedef struct __GainControlUnit
{
	uint16_t      enable;
	uint16_t      mute;
	uint16_t      gain;
	uint16_t      lastgain;
	uint8_t       channel;
} GainControlUnit;

typedef struct __FlangerUnit
{
	void             *ct;
	int16_t          enable;
	int16_t          delay_length;
	int16_t          mod_depth;
	int16_t  		 mod_rate;
	int16_t          feedback;
	int16_t  		 dry;
	int16_t  		 wet;
	int16_t          bit_width;
} FlangerUnit;

typedef struct __OverdriveUnit
{
	OverdriveContext  *ct;
	int16_t          enable;
	int16_t          threshold_compression;
} OverdriveUnit;

typedef struct __OverdrivePolyUnit
{
	OverdrivePolyContext  *ct;
	int16_t          enable;
	int16_t          gain;
	int16_t          out_level;
} OverdrivePolyUnit;
typedef struct __DistortionUnit
{
	DistortionExpContext  *ct;
	int16_t          enable;
	int16_t          gain;
	int16_t  		 dry;
	int16_t  		 wet;
} DistortionUnit;
typedef struct __DistortionDS1Unit
{
	DistortionDS1Context  *ct;
	int16_t          enable;
	int16_t          distortion_level;
	int16_t          out_level;
} DistortionDS1Unit;

typedef struct __PitchDetectorUnit
{
	PitchDetectorContext *ct;
	int16_t          enable;
	int16_t          pitch_min;
	int16_t  		 pitch_max;
	int16_t  		 window_size;
	int16_t          step_size;
	int16_t          confidence_threshold;
	int16_t          confidence;
	uint32_t  		 sample_rate;
	uint32_t         freq;
	uint32_t         freq_bak;
	uint8_t          channel;

} PitchDetectorUnit;

typedef struct __BlueNsUnit
{
	void                 *ct;
	int16_t              enable;
	int16_t 			 ns_level;
	int16_t              block_size;
} BlueNsUnit;
//----------------------//

typedef struct __EQDRCUnit
{
	EQDRCContext  *ct;
	int16_t       enable;
	int16_t       reserve1;
	int16_t       reserve2;
	//eq-----------------------------//
	FilterParams  eq_params[10];
	//drc-----------------------------//
	int16_t 	  drc_mode;
	int16_t       cf_type;
	int16_t 	  q_l;
	int16_t 	  q_h;
	int32_t 	  fc[2];
	int32_t  	  threshold[4];
	int32_t  	  ratio[4];
	int32_t  	  attack_tc[4];
	int32_t  	  release_tc[4];
	int32_t  	  pregain[4];
	//eq------------------------------//
	EQFilterParams  *filter_params;
	uint16_t 	  filter_count;
	uint8_t       channel;

} EQ_DRCUnit;
//------------------------------------------------//
typedef struct __DRCLegacyUnit
{
	DRCLegacyContext *ct;
	int16_t 		 enable;
	int16_t 		 fc;
	int16_t 		 mode;
	uint16_t 		 q[2];
	int16_t  		 threshold[3];
	int16_t  		 ratio[3];
	int16_t  		 attack_tc[3];
	int16_t  		 release_tc[3];
	int16_t  		 pregain1;
	int16_t  		 pregain2;
	uint8_t          channel;
}DRCLegacyUnit;
//-----------------------------------//
typedef struct __FaderUnit
{
	FaderContext  *in_ct;
	FaderContext  *out_ct;
	int16_t       enable;
	int16_t       pitch_min;
} FaderUnit;
//---------------------------------------//
typedef struct __CompanderUnit
{
	CompanderContext  *ct;
	int16_t            enable;
	int16_t 		   threshold;
	int16_t            ratio_below;
	int16_t 		   ratio_above;
	int16_t            attack_time;
	int16_t 		   release_time;
	int16_t            pregain;
	int16_t            channel;
} CompanderUnit;

typedef struct __LowLevelCompressorUnit
{
	LowLevelCompressorContext  *ct;
	int16_t                    enable;
	int16_t 		           threshold;
	int16_t                    gain;
	int16_t                    attack_time;
	int16_t 		           release_time;
	int16_t                    channel;
} LowLevelCompressorUnit;


typedef struct __HowlingGuardDescribe
{
	ParaNumbers          numbers;//totals
	TypeContinue         P1;//saturation_threshold
	TypeContinue         P2;//high_freq_threshold;
	TypeContinue         P3;//high_freq_energy_ratio_threshold;
	TypeContinue         P4;//max_saturated_high_freq_duration;
	TypeContinue         P5;//max_saturated_duration;
	TypeContinue         P6;//mute_period;
	TypeContinue         P7;//noise_gate_threshold;
}HowlingGuardDescribe;

typedef struct __HowlingGuardUnit
{
	HowlingGuardContext  *ct;
	int16_t 			 enable;
	int16_t 			 saturation_threshold;
	int16_t  	 		 high_freq_threshold;
	int16_t   	 		 high_freq_energy_ratio_threshold;
	int16_t   	 		 max_saturated_high_freq_duration;
	int16_t 		     max_saturated_duration;
	int16_t              mute_period;
	int16_t 		     noise_gate_threshold;
} HowlingGuardUnit;


typedef struct __PhaseShifterDescribe
{
	ParaNumbers          numbers;//totals
	TypeContinue         P1;//phase_shift
}PhaseShifterDescribe;

typedef struct __PhaseShifterUnit
{
	PhaseShifterContext  *ct;
	int16_t 			 enable;
	int16_t  	 		 phase_shift;
	int16_t              step_size;
	int16_t 		     channel;
} PhaseShifterUnit;

//------DRA POST-------------------------------------//
typedef struct __DraPostDescribe1
{
	ParaNumbers          numbers;//totals
	TypeBool             P1;//int16_t nEffectActive; (0~1)
	TypeContinue         P2;//int16_t nFreq1;(0~1000)hz
	TypeContinue         P3;//int16_t nFreq2;(0~1000)hz
	TypeContinue         P4;//int16_t nFreq3;(1000~20000)hz
	TypeContinue         P5;//int16_t nFreq4;(1000~20000)hz
	TypeContinue         P6;//int16_t nWidenCenter;(0~90)C
	TypeContinue         P7;//int16_t nWidenGain;(1~19)db-level
	TypeContinue         P8;//int16_t nTotalGain;(10~20)db-level
}DraPostDescribe1;

typedef struct __DraPostDescribe2
{
	ParaNumbers          numbers;//totals
	TypeBool             P9;//int16_t  bCTCActive; (0~1)CTC SW
	TypeContinue         P10;//int16_t nCTCMode;   (1~12)CTC MODE
	//-------------------------------//
}DraPostDescribe2;

typedef struct __DraPostDescribe3
{
	ParaNumbers          numbers;//totals
	TypeBool             P11;//int16_t bUpmixActive; (0~1)
	TypeContinue         P12;//int16_t fUpmixLRGainCoef;(0~20)db			// 10x
	TypeContinue         P13;//int16_t fUpmixTlTrGainCoef;(0~20)db			// 10x
	TypeContinue         P14;//int16_t fUpmixGainCoef;	(0~20)db			// 10x
	TypeContinue         P15;//int16_t fSpeechEnhancementGain; 	(10~20)db// 10x
}DraPostDescribe3;

typedef struct __DraPostDescribe4
{
	ParaNumbers          numbers;//totals
	TypeBool             P16;//int16_t bVirtualBassActive;(0~1) SW
	//------------------------------------------------//
	TypeContinue         P17;//int16_t nVBCutOffFreq;(60, 80, 100, 120, 140, 160, 180)HZ
	TypeContinue         P18;//int16_t fVBGain;(1~10)db
	TypeEnum             P19;//int16_t nLowFreq1;(30, 40, 55, 70)hz
	TypeContinue         P20;//int16_t nLowFreq2;(60, 80, 100, 120, 140, 160, 180)HZ
}DraPostDescribe4;

typedef struct __DraPostDescribe5
{
	ParaNumbers          numbers;//totals
	//-------------------------------//
	TypeEnum             P21;//int16_t nHarmCutOff;(240, 300, 400, 500, 600, 700, 800)hz
	TypeContinue         P22;//int16_t fScale; 	(1~10)db					// 10x
	TypeContinue         P23;//int16_t fGain_f1; (1~10)db					// 10x
	TypeContinue         P24;//int16_t nVolume;   (1~32)db
	//---------------------------------------------//
	TypeContinue         P25;//int16_t nDistance;(50~500)
	TypeEnum             P26;//int16_t nRoomMode;(1,2,3),H,M,L
}DraPostDescribe5;
//---------------------------------//
typedef struct __DraPostUnit
{
	DraPostProcessingContext     *ct;
	DraPostProcessingParam       parameter;
	int16_t 			         enable;
	//
	int16_t nEffectActive;
	int16_t nFreq1;
	int16_t nFreq2;
	int16_t nFreq3;
	int16_t nFreq4;
	int16_t nWidenCenter;
	int16_t nWidenGain;
	int16_t nTotalGain;

	int16_t bCTCActive;
	int16_t nCTCMode;
	int16_t bUpmixActive;
	int16_t fUpmixLRGainCoef;			// 10x
	int16_t fUpmixTlTrGainCoef;			// 10x
	int16_t fUpmixGainCoef;				// 10x
	int16_t fSpeechEnhancementGain; 	// 10x
	int16_t bVirtualBassActive;

	int16_t nVBCutOffFreq;
	int16_t fVBGain;
	int16_t nLowFreq1;
	int16_t nLowFreq2;
	int16_t nHarmCutOff;
	int16_t fScale; 						// 10x
	int16_t fGain_f1; 					// 10x
	int16_t nVolume;

	int16_t nDistance;
	int16_t nRoomMode;
	int16_t nReserverd1;
	int16_t nReserverd2;
	//----------------//
	int16_t channel;
} DraPostUnit;
//-----------------------------------------//
typedef struct __DCBlockerDescribe
{
	ParaNumbers          numbers;//totals
	TypeContinue         P1;//reserve
}DCBlockerDescribe;

typedef struct __DCBlockerUnit
{
	DCBlocker            *ct;
	int16_t 			 enable;
	int16_t 			 reserve;
	int16_t              channel;
} DCBlockerUnit;

//-----------------------------------------//
typedef struct __VirtualSurroundDescribe
{
	ParaNumbers          numbers;//totals
	TypeContinue         P1;//reserve
}VirtualSurroundDescribe;

typedef struct __VirtualSurroundUnit
{
	VirtualSurroundContext     *ct;
	int16_t 			       enable;
	int16_t 			       reserve;
	uint32_t                   SampleRate;
	int16_t                    channel;
} VirtualSurroundUnit;

//--------------------------------------//
typedef struct __NoiseGeneratorDescribe
{
	ParaNumbers          numbers;//totals
	TypeContinue         P1;//type
	TypeContinue         P2;//amp
}NoiseGeneratorDescribe;

typedef struct __NoiseGeneratorUnit
{
	NoiseGeneratorContext  *ct;
	int16_t 			 enable;
	int16_t  	 		 noise_type;
	int16_t              amplitude;
	int16_t 		     channel;
} NoiseGeneratorUnit;

//--------------------------------------//
typedef struct __ButterWorthDescribe
{
	ParaNumbers          numbers;//totals
	TypeEnum             P1;//filter_type
	TypeContinue         P2;//filter_order
	TypeContinue         P3;//<sample rate/2
}ButterWorthDescribe;

typedef struct __ButterWorthUnit
{
	uint8_t              *ct;
	int16_t 			 enable;
	int16_t  	 		 filter_type; //0,1
	int16_t              filter_order;//1~10
	int16_t              fc;          // <sample rate/2
	int16_t 		     channel;
}ButterWorthUnit;
//--------------------------------------//
typedef struct __DynamicEqDescribe
{
	ParaNumbers          numbers;//totals
	TypeContinue         P1;//low_energy_threshold
	TypeContinue         P2;//normal_energy_threshold
	TypeContinue         P3;//high_energy_threshold
	TypeContinue         P4;//attack_time
	TypeContinue         P5;//release_time
}DynamicEqDescribe;

typedef struct __DynamicEqUnit
{
	DynamicEQContext     *ct;
	EQUnit               eq_low;
	EQUnit               eq_high;
	EQUnit               eq_watch;
	int16_t 			 enable;
	int16_t  	 		 low_energy_threshold; //-9000 ~ 0 to cover -90.00dB ~ 0.00dB
	int16_t              normal_energy_threshold;//-9000 ~ 0 to cover -90.00dB ~ 0.00dB
	int16_t              high_energy_threshold;//-9000 ~ 0 to cover -90.00dB ~ 0.00dB
	int16_t              attack_time;          //0~1000ms
	int16_t              release_time;         //0~1000ms
	int16_t 		     channel;
	EQFilterParams       low_filter_buf[10];
	EQFilterParams       high_filter_buf[10];
	EQFilterParams       watch_filter_buf[10];
}DynamicEqUnit;
//--------------------------------------//
typedef struct __LRBalancerDescribe
{
	ParaNumbers          numbers;//totals
	TypeContinue         P1;//+100 ~ -100
}LRBalancerDescribe;

typedef struct __LRBalancerUnit
{
	LRBalancerContext     *ct;
	int16_t 			 enable;
	int16_t  	 		 balance; //range: -100 ~ 100.
	int16_t 		     channel;
}LRBalancerUnit;
//--------------------------------------//
typedef struct __HowlingSpecifiedDescribe
{
	ParaNumbers          numbers;//totals
	TypeContinue         P1;//0~6 = MAX_SPECIFIED_FILTERS

	TypeContinue         P2;////Range: 2 ~ sample_rate/2-2 Hz.
	TypeContinue         P3;////Range: 2 ~ sample_rate/2-2 Hz.
	TypeContinue         P4;////Range: 2 ~ sample_rate/2-2 Hz.
	TypeContinue         P5;////Range: 2 ~ sample_rate/2-2 Hz.
	TypeContinue         P6;////Range: 2 ~ sample_rate/2-2 Hz.
	TypeContinue         P7;////Range: 2 ~ sample_rate/2-2 Hz.

	TypeContinue         P8;//////Range: 1 ~ 32767
	TypeContinue         P9;//////Range: 1 ~ 32767
	TypeContinue         P10;//////Range: 1 ~ 32767
	TypeContinue         P11;//////Range: 1 ~ 32767
	TypeContinue         P12;//////Range: 1 ~ 32767
	TypeContinue         P13;//////Range: 1 ~ 32767

	TypeContinue         P14;//Range: -100 ~ 0 dB.
	TypeContinue         P15;//Range: -100 ~ 0 dB.
	TypeContinue         P16;//Range: -100 ~ 0 dB.
	TypeContinue         P17;//Range: -100 ~ 0 dB.
	TypeContinue         P18;//Range: -100 ~ 0 dB.
	TypeContinue         P19;//Range: -100 ~ 0 dB.
}HowlingSpecifiedDescribe;

typedef struct __HowlingSpecifiedUnit
{
	HowlingSpecifiedContext     *ct;
	int16_t 			         enable;
	int16_t  	 		         num_specified_filters; //0~6 = MAX_SPECIFIED_FILTERS
	int16_t                      center_freq[6];          //Range: 2 ~ sample_rate/2-2 Hz.
	int16_t                      q[6];                    //Range: 1 ~ 32767
	int16_t                      depth[6];                //Range: -100 ~ 0 dB.
}HowlingSpecifiedUnit;

//--------------------------------------//
typedef struct __VADDescribe
{
	ParaNumbers    numbers;//totals
	TypeBool       P1;//int16_t 0:disabled, 1:enabled.

}VADDescribe;

typedef struct __VADUnit
{
	uint8_t      *ct;
	uint8_t      *scratch;//scratch_buf
	int16_t 	  enable;
	int16_t  	  post_processing; //int16_t 0:disabled, 1:enabled.
	//---------------------------------------------------//
	int16_t       voiced_status;//1=voiced, 0=no voiced

}VADUnit;

//-----system var--------------------------//
typedef struct _ControlVariablesContext
{
	//for system control 0x01
	uint16_t            sys_mode;
	uint16_t            sys_reset;
	uint16_t            sys_sample_rate_en;
	uint16_t 		    sys_sample_rate;
	uint16_t            sys_mclk_src_en;
	uint16_t            sys_mclk_src;
	uint16_t            sys_default_set;
	
	//for System status 0x02
    uint16_t 			cpu_mips;
	uint16_t 			UsedRamSize;
	uint16_t 			AutoRefresh;//调音时音效参数发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取
	uint16_t            CpuMaxFreq;
	uint16_t            CpuMaxRamSize;
    
	//for ADC0 PGA      0x03
    uint16_t            pga0_line1_l_en;
    uint16_t            pga0_line1_r_en;
    uint16_t            pga0_line2_l_en;
    uint16_t            pga0_line2_r_en;
    uint16_t            pga0_line4_l_en;
    uint16_t            pga0_line4_r_en;
    uint16_t            pga0_line5_l_en;
    uint16_t            pga0_line5_r_en;

	uint16_t            pga0_line1_pin;
    uint16_t            pga0_line2_pin;
    uint16_t            pga0_line4_pin;
    uint16_t            pga0_line5_pin;
	
    uint16_t            pga0_line1_l_gain;
    uint16_t            pga0_line1_r_gain;
    uint16_t            pga0_line2_l_gain;
    uint16_t            pga0_line2_r_gain;
    uint16_t            pga0_line4_l_gain;
    uint16_t            pga0_line4_r_gain;
    uint16_t            pga0_line5_l_gain;
    uint16_t            pga0_line5_r_gain;
	uint16_t            pga0_diff_mode;
	uint16_t            pga0_diff_l_gain;
	uint16_t            pga0_diff_r_gain;

	uint16_t            pga0_zero_cross;
	
    //for ADC0 DIGITAL  0x04
    uint16_t         	adc0_channel_en;
	uint16_t  			adc0_mute;
    uint16_t  			adc0_dig_l_vol;
    uint16_t  			adc0_dig_r_vol;
	uint16_t  			adc0_sample_rate;
	uint16_t  			adc0_lr_swap;
	uint16_t            adc0_dc_blocker;
	uint16_t            adc0_fade_time;
	uint16_t            adc0_mclk_src;
	uint16_t            adc0_dc_blocker_en;
	
	//for AGC0 ADC0     0x05
	uint16_t            adc0_agc_mode;
	uint16_t            adc0_agc_max_level;
	uint16_t            adc0_agc_target_level;
	uint16_t            adc0_agc_max_gain;
	uint16_t            adc0_agc_min_gain;
	uint16_t            adc0_agc_gainoffset;
	uint16_t            adc0_agc_fram_time;
	uint16_t            adc0_agc_hold_time;
	uint16_t            adc0_agc_attack_time;
	uint16_t            adc0_agc_decay_time;
	uint16_t            adc0_agc_noise_gate_en;
	uint16_t            adc0_agc_noise_threshold;
	uint16_t            adc0_agc_noise_gate_mode;
	uint16_t            adc0_agc_noise_time;
	
	//for ADC1 PGA      0x06 
	uint16_t             line3_l_mic1_en;
	uint16_t             line3_r_mic2_en;
    uint16_t             line3_l_mic1_gain;
	uint16_t             line3_l_mic1_boost;
    uint16_t             line3_r_mic2_gain;	
    uint16_t             line3_r_mic2_boost;	
	uint16_t             mic_or_line3;
	uint16_t             mic1_line3l_pin_en;
	uint16_t             mic2_line3r_pin_en;

	//for ADC1 DIGITAL  0x07
    uint16_t         	adc1_channel_en;
	uint16_t  			adc1_mute;
    uint16_t  			adc1_dig_l_vol;
    uint16_t  			adc1_dig_r_vol;
	uint16_t  			adc1_sample_rate;
	uint16_t  			adc1_lr_swap;
	uint16_t            adc1_dc_blocker;
	uint16_t            adc1_fade_time;
	uint16_t            adc1_mclk_src;
	uint16_t            adc1_dc_blocker_en;

	//for AGC1  ADC1    0x08
	uint16_t            adc1_agc_mode;
	uint16_t            adc1_agc_max_level;
	uint16_t            adc1_agc_target_level;
	uint16_t            adc1_agc_max_gain;
	uint16_t            adc1_agc_min_gain;
	uint16_t            adc1_agc_gainoffset;
	uint16_t            adc1_agc_fram_time;
	uint16_t            adc1_agc_hold_time;
	uint16_t            adc1_agc_attack_time;
	uint16_t            adc1_agc_decay_time;
	uint16_t            adc1_agc_noise_gate_en;
	uint16_t            adc1_agc_noise_threshold;
	uint16_t            adc1_agc_noise_gate_mode;
	uint16_t            adc1_agc_noise_time;

	//for DAC0          0x09
	uint16_t            dac0_en;
	uint16_t            dac0_sample_rate;
	uint16_t            dac0_dig_mute;
	uint16_t            dac0_dig_l_vol;
	uint16_t            dac0_dig_r_vol;
	uint16_t            dac0_dither;
	uint16_t            dac0_scramble;
	uint16_t            dac0_out_mode;
	uint16_t            dac0_pause_en;
	uint16_t            dac0_sample_mode;
	uint16_t            dac0_scf_mute; 
	uint16_t            dac0_fade_time;
	uint16_t            dac0_zeros_number;
	uint16_t            dac0_mclk_src;
	uint16_t            dac0_out_bit_len;
	
	//for DAC1          0x0a
	uint16_t            dac1_en;
	uint16_t            dac1_sample_rate;
	uint16_t            dac1_dig_mute;
	uint16_t            dac1_dig_l_vol;
	uint16_t            dac1_dig_r_vol;
	uint16_t            dac1_dither;
	uint16_t            dac1_scramble;
	uint16_t            dac1_out_mode;
	uint16_t            dac1_pause_en;
	uint16_t            dac1_sample_mode;
	uint16_t            dac1_scf_mute; 
	uint16_t            dac1_fade_time;
	uint16_t            dac1_zeros_number;
	uint16_t            dac1_mclk_src;
	uint16_t            dac1_out_bit_len;

	//for i2s0          0x0b
	uint16_t            i2s0_tx_en;
	uint16_t            i2s0_rx_en;
	uint16_t            i2s0_sample_rate;
	uint16_t            i2s0_mclk_src;
	uint16_t            i2s0_work_mode;
	uint16_t            i2s0_word_len;
	uint16_t            i2s0_stereo_mono;
	uint16_t            i2s0_fade_time;
	uint16_t            i2s0_format;
	uint16_t            i2s0_bclk_invert_en;
	uint16_t            i2s0_lrclk_invert_en;
	
	//for i2s1          0x0c
	uint16_t            i2s1_tx_en;
	uint16_t            i2s1_rx_en;
	uint16_t            i2s1_sample_rate;
	uint16_t            i2s1_mclk_src;
	uint16_t            i2s1_work_mode;
	uint16_t            i2s1_word_len;
	uint16_t            i2s1_stereo_mono;
	uint16_t            i2s1_fade_time;
	uint16_t            i2s1_format;
	uint16_t            i2s1_bclk_invert_en;
	uint16_t            i2s1_lrclk_invert_en;

	//for spdif
	uint16_t  			spdif_en;
	uint16_t  			spdif_sample_rate;
	uint16_t  			spdif_channel_mode;
	uint16_t  			spdif_in_gpio;
	uint16_t  			spdif_lock_status;

    //for Audio Effects
	BlueNsUnit          hfp_ns_unit;
	AecUnit             mic_aec_unit;
	EQUnit	   			eq_for_aec_unit;
	ExpanderUnit		expander_for_aec_unit;
	DRCUnit				drc_for_aec_unit;
	GainControlUnit		aec_gain_control_unit;
	GainControlUnit		aec_mic_in_gain_unit;
	GainControlUnit		aec_phone_in_gain_unit;
	
	#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
    PcmDelayUnit        music_delay_unit;
	#endif
	ExciterUnit         music_exciter_unit;
    AutoTuneUnit 		auto_tune_unit;
    DRCUnit 	   		music_drc_unit;
	DRCUnit 	   		mic_drc_unit;

    #if CFG_AUDIO_EFFECT_DRC_LEGACY_EN
	DRCLegacyUnit 	    music_drc_legacy_unit;
	DRCLegacyUnit 	    mic_drc_legacy_unit;
    #endif

    DRCUnit     		rec_drc_unit;
    EchoUnit     		echo_unit;
    EchoUnit            guitar_echo_unit;
    ExpanderUnit		music_expander_unit;
    ExpanderUnit		mic_expander_unit;
    #if CFG_AUDIO_EFFECT_MIC_BLUENS_SUPPRESSOR_EN
    BlueNsUnit          mic_ns_unit;
    #endif
	#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
	ThreeDUnit          music_threed_unit;
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
	ThreeDPlusUnit      music_threed_plus_unit;
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
	VBUnit              music_vb_unit;
	#endif
	#if CFG_AUDIO_EFFECT_REC_VIRTUAL_BASS_EN
	VBUnit              rec_vb_unit;
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
	VBClassicUnit       music_vb_classic_unit;
	#endif
    PitchShifterUnit    pitch_shifter_unit;
	#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
	PitchShifterProUnit pitch_shifter_pro_unit;
	#endif
    ReverbUnit  		reverb_unit;
	ReverbPlateUnit     plate_reverb_unit;
	#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
	ReverbProUnit       reverb_pro_unit;
	#endif
    VoiceChangerUnit    voice_changer_unit;
	#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
	VoiceChangerProUnit voice_changer_pro_unit;
	#endif
	FreqShifterUnit     freq_shifter_unit;
	FreqShifterFineUnit freq_shifter_fine_unit;
	HowlingDectorUnit   howling_dector_unit;
    #if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
	HowlingDectorFineUnit howling_dector_fine_unit;
    #endif
    #if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
	HowlingSpecifiedUnit howling_dector_specified_unit;
    #endif
	SilenceDetectorUnit MicAudioSdct_unit;
	SilenceDetectorUnit MusicAudioSdct_unit;
	VocalCutUnit        vocal_cut_unit;
	VocalRemoveUnit     vocal_remove_unit;
	ChorusUnit          chorus_unit;
	Chorus2Unit         chorus2_unit;
    EQUnit	   			music_pre_eq_unit;
	EQUnit	   			music_out_eq_unit;
    EQUnit	   			mic_pre_eq_unit;
    EQUnit	   			mic_bypass_eq_unit;
    EQUnit	   			mic_echo_eq_unit;
    EQUnit	   			mic_reverb_eq_unit;
    EQUnit	   			mic_out_eq_unit;
    EQUnit	   			rec_eq_unit;
    EQUnit	   			guitar_eq_unit;
    BiquadUnit          biquad_unit;
	#if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN	
    EQ_DRCUnit          music_eq_drc_unit;
    EQ_DRCUnit          rec_eq_drc_unit;
	#endif
	PhaseControlUnit    phase_control_unit;
	StereoWindenUnit    stereo_winden_unit;
	PingPongUnit        ping_pong_unit;
	AutoWahUnit         auto_wah_unit;
	FlangerUnit         flanger_uint;
    #if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
	DistortionDS1Unit   distortion_ds1_unit;
    #endif

    #if CFG_AUDIO_EFFECT_OVERDRIVER_POLY_EN
	OverdrivePolyUnit   overdrive_poly_unit;
    #endif
    #if CFG_AUDIO_EFFECT_OVERDRIVER_EN
	OverdriveUnit       overdrive_unit;
    #endif
    #if CFG_AUDIO_EFFECT_DISTORTION_EN
    DistortionUnit      distortion_unit;
    #endif
    #if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
	PitchDetectorUnit   pitch_detector_unit;
	#endif
    #if CFG_AUDIO_EFFECT_COMPANDER_EN
	CompanderUnit       compander_unit;
    #endif

    #if CFG_AUDIO_EFFECT_DRAPOST_EN
	DraPostUnit        dra_post_unit;
    #endif

	#if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
	LowLevelCompressorUnit mic_low_level_compressor_unit;
	#endif

    #if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
	HowlingGuardUnit       mic_howling_guard_unit;
    #endif

    #if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
	PhaseShifterUnit  rec_phase_shifter_unit;
    #endif
    #if CFG_AUDIO_EFFECT_DC_BLOCKER_EN
	DCBlockerUnit      dc_blocker_unit;
    #endif
    #if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
	VirtualSurroundUnit    virtual_surround_unit;
    #endif
    #if CFG_AUDIO_EFFECT_NOISE_GENERATOR_EN
	NoiseGeneratorUnit    noise_generator_unit;
    #endif
	//
    #if CFG_AUDIO_EFFECT_BUTTERWORTH_EN
	ButterWorthUnit    music_butterworth_unit;
    #endif
	//
    #if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN
	DynamicEqUnit    music_dynamic_eq_unit;
    #endif
    #if CFG_AUDIO_EFFECT_VAD_EN
	VADUnit            music_vad_unit;
    #endif
	//gain control
    GainControlUnit		aux_gain_control_unit;
    GainControlUnit		mic_bypass_gain_control_unit;
    GainControlUnit		mic_echo_control_unit;
    GainControlUnit		mic_reverb_gain_control_unit;
    GainControlUnit		mic_out_gain_control_unit;
    GainControlUnit		rec_bypass_gain_control_unit;
	GainControlUnit		rec_effect_gain_control_unit;
    GainControlUnit		rec_aux_gain_control_unit;
	GainControlUnit		rec_remind_gain_control_unit;
    GainControlUnit		rec_out_gain_control_unit;
    GainControlUnit		remind_key_gain_control_unit;
    GainControlUnit		remind_effect_gain_control_unit;
	GainControlUnit		i2s_gain_control_unit;
	GainControlUnit		bt_gain_control_unit;
	GainControlUnit		spdif_gain_control_unit;
	GainControlUnit		usb_gain_control_unit;
	GainControlUnit		Fade_gain_control_unit;
	GainControlUnit		rec_usb_out_gain_control_unit;
	GainControlUnit		guitar_gain_control_unit;
    #ifdef CFG_AUDIO_EFFECT_FADER_EN
	FaderUnit          MusicFaderCt;
    #endif	
	#ifdef CFG_FUNC_TSM_EN
	TSMUnit             tsm_unit;
    #endif

    #if CFG_AUDIO_EFFECT_LRBALANCER_EN
	LRBalancerUnit     music_lr_balancer;
    #endif
    //for system define
	uint16_t            sample_rate;
	uint16_t            crypto_en;
	uint32_t            password;
	uint8_t             IsEffectChangedByPcTool;
	uint8_t             adc_mic_channel_num;//for mic
	uint8_t             adc_line_channel_num;///for line ,fm ,bt ,usb ,i2s
	#if ((CFG_RES_MIC_SELECT == 0) && defined(BT_TWS_SUPPORT))
	uint8_t             line_num;
	#endif
	uint8_t             remind_type;
	uint8_t             UsbAudioMute;
	uint8_t             usb_audio_upload_flag;
	uint16_t            SamplesPerFrame;
	uint8_t             audio_effect_init_flag;
	uint16_t            beep_en;
	uint16_t            beepAddr;
	#ifdef CFG_FUNC_SHUNNING_EN
	uint8_t             ShunningMode;
	uint32_t            aux_out_dyn_gain;
	uint32_t            remind_out_dyn_gain;
	uint32_t            usb_in_dyn_gain;
	#endif
	#ifdef CFG_FUNC_DETECT_MIC_EN
	uint8_t             MicOnlin;
	uint8_t             Mic2Onlin;
	#endif
	#ifdef CFG_FUNC_DETECT_MIC_SEG_EN
	uint8_t             MicSegment;
	#endif
	#ifdef CFG_FUNC_DETECT_PHONE_EN
	uint8_t             EarPhoneOnlin;
	#endif
	uint16_t            UsbAudioVolume;
	uint16_t            UsbMicVolume;
	uint16_t            UsbMicMute;
    //------音效参数的最大值 来自调音参数数据-------------------------------//	
    uint32_t            max_chorus_wet;
    uint32_t            max_plate_reverb_roomsize;
    uint32_t            max_plate_reverb_wetdrymix;
	uint32_t            max_echo_delay;
	uint32_t            max_echo_depth;
	uint32_t 		    max_reverb_wet_scale;
	uint32_t 		    max_reverb_roomsize;
	int32_t			    max_reverb_pro_wet;
    int32_t             max_reverb_pro_erwet;	

}ControlVariablesContext;
//----------------------------------------//
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
#ifdef CFG_RES_USE_EQ_DRC_TREB_BASS_EN
   EQ_DRCUnit	   	    *music_treb_bass_eq_unit;
   #else
   EQUnit	   			*music_treb_bass_eq_unit;
   #endif
#endif
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
EQUnit	   			*music_eq_mode_unit;
#endif
#ifdef CFG_FUNC_MIC_TREB_BASS_EN
EQUnit	   			*mic_treb_bass_eq_unit;
#endif
//----------------------------------------//
EQFilterParams eq1_filter_buf[10];
EQFilterParams eq2_filter_buf[10];
EQFilterParams eq3_filter_buf[10];
EQFilterParams eq4_filter_buf[10];
EQFilterParams eq5_filter_buf[10];
EQFilterParams eq6_filter_buf[10];
EQFilterParams eq7_filter_buf[10];
EQFilterParams eq8_filter_buf[10];

EQFilterParams biquad_filter_buf[1];

#if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN	
EQFilterParams eq_drc_1_filter_buf[10];
EQFilterParams eq_drc_2_filter_buf[10];
extern EQ_DRCUnit *eq_drc_unit[2];
#endif

EQFilterParams eq_hfp_filter_buf[10];
extern ControlVariablesContext gCtrlVars;

//---system const-----------------------------//
extern const uint32_t SupportSampleRateList[13];
extern const uint8_t AutoTuneKeyTbl[13];
extern const uint8_t AutoTuneSnapTbl[3];
extern const uint8_t MIC_BOOST_LIST[5];
extern const int16_t DeltafTable[8];
#ifdef BT_TWS_SUPPORT
extern const AUDIO_EFF_PARAMAS EFFECT_TAB[18];
#else
extern const AUDIO_EFF_PARAMAS EFFECT_TAB[11];
#endif
extern const AUDIO_EFF_PARAMAS FLASH_EFFECT_TAB[5];
extern const int32_t DRC_DEFAULT_TABLE[][3];
extern const int32_t EQ_DEFAULT_TABLE[][8];
extern const uint16_t DigVolTab_256[256];
extern const int16_t DigVolTab_64[64];
extern const int16_t DigVolTab_32[32];
extern const int16_t DigVolTab_16[16];
extern const int16_t EchoDlyTab_16[16];
extern const uint8_t ReverbRoomTab[16];
extern const uint8_t PlateReverbRoomTab[16];
extern const int32_t EXPANDER_DEFAULT_TABLE[][2];
extern const int32_t GAIN_CONTROL_TABLE[][16];
extern const int16_t BassTrebGainTable[16];
extern const unsigned char HunXiang[2907];
extern const unsigned char HanMai[2907];
extern const unsigned char DianYin[2907];
extern const unsigned char MoYin[2907];
extern const unsigned char NanBianNv[2907];
extern const unsigned char NvBianNan[2907];
extern const unsigned char WaWaYin[2907];
extern const unsigned char YuanSheng[2907];
extern const unsigned char Music[1468];
#ifdef BT_TWS_SUPPORT
extern const unsigned char HunXiang_Slave[2907];
extern const unsigned char HanMai_Slave[2907];
extern const unsigned char DianYin_Slave[2907];
extern const unsigned char MoYin_Slave[2907];
extern const unsigned char NanBianNv_Slave[2907];
extern const unsigned char NvBianNan_Slave[2907];
extern const unsigned char WaWaYin_Slave[2907];
extern const unsigned char Music_Slave[1468];
#endif
extern const unsigned char AECBuf[599];
extern const unsigned char UsbAecBuf[599];

//-----system function--------------------------//
extern EQUnit *eq_unit_aggregate[8];
extern DRCUnit *drc_unit_aggregate[3];
extern EQFilterParams *eq_param_aggregate[8];
extern ExpanderUnit *expander_unit_aggregate[2];
extern GainControlUnit *gain_unit_aggregate[16];
extern DRCLegacyUnit *drc_legacy_unit_init[2];

void CtrlVarsInit(void);
void DefaultParamgsInit(void);
void Line3MicPinSet(void);
void GlobalSampeRateSet(void);
void CpuVerification(void);
void GlobalSampeRateSet(void);
void GlobalMclkSet(void);
void UsbLoadAudioMode(uint16_t len,uint8_t *buff);
bool  AudioEffectListJudge(uint16_t len, const uint8_t *buff);
void Communication_Effect_Init(void);
void AudioLineSelSet(void);
void BeepEnable(void);
void Beep(int16_t *OutBuf,uint16_t n);
void AudioAnaChannelSet(int8_t ana_input_ch);
void MicLine3PinSet(void);
void AudioLine3MicSelect(void);
void SamplesFrameUpdataMsg(void);
void EffectUpdataMsg(void);
/***********     Audio effects  function  declaration **************/
void AudioEffectPcmDelayInit(PcmDelayUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width);

void AudioEffectExciterInit(ExciterUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectAecInit(AecUnit *unit,  uint32_t sample_rate);
void AudioEffectExpanderInit(ExpanderUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectFreqShifterInit(FreqShifterUnit *unit);
void AudioEffectFreqShifterFineInit(FreqShifterFineUnit *unit, int32_t sample_rate);
void AudioEffectHowlingSuppressorInit(HowlingDectorUnit *unit);
void AudioEffectPitchShifterInit(PitchShifterUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width);
#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
void AudioEffectPitchShifterProInit(PitchShifterProUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
void AudioEffectAutoTuneInit(AutoTuneUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width);
void AudioEffectVoiceChangerInit(VoiceChangerUnit *unit, uint8_t channel, uint32_t sample_rate);
#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
void AudioEffectVoiceChangerProInit(VoiceChangerProUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
void AudioEffectEchoInit(EchoUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width);
void AudioEffectReverbInit(ReverbUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width);
void AudioEffectPlateReverbInit(ReverbPlateUnit *unit, uint8_t channel, uint32_t sample_rate);
#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
void AudioEffectReverbProInit(ReverbProUnit *unit, uint8_t channel, uint32_t sample_rate,uint32_t bit_width);
#endif
void AudioEffectDRCInit(DRCUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectEQInit(EQUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectSilenceDectorInit(SilenceDetectorUnit *unit,uint8_t channel, uint32_t sample_rate);
void AudioEffectVocalCutInit(VocalCutUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectVocalRemoveInit(VocalRemoveUnit *unit,  uint8_t channel, uint32_t sample_rate,uint8_t bit_width);
void AudioEffectChorusInit(ChorusUnit *unit,  uint32_t sample_rate);
void AudioEffectChorus2Init(Chorus2Unit *unit,  uint32_t sample_rate,uint16_t bit_width);
#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
void AudioEffectThreeDInit(ThreeDUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
void AudioEffectThreeDPlusInit(ThreeDPlusUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
void AudioEffectVBInit(VBUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
void AudioEffectVBClassicInit(VBClassicUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
void AudioEffectPcmDelayConfig(PcmDelayUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
void AudioEffectExciterConfig(ExciterUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectExpanderThresholdConfig(ExpanderUnit *unit);
void AudioEffectExpanderConfig(ExpanderUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectFreqShifterConfig(FreqShifterUnit *unit);
void AudioEffectFreqShifterFineConfig(FreqShifterFineUnit *unit);
void AudioEffectPitchShifterConfig(PitchShifterUnit *unit);
#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
void AudioEffectPitchShifterProConfig(PitchShifterProUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
void AudioEffectAutoTuneConfig(AutoTuneUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectVoiceChangerConfig(VoiceChangerUnit *unit, uint8_t channel, uint32_t sample_rate);
#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
void AudioEffectVoiceChangerProConfig(VoiceChangerProUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
void AudioEffectEchoConfig(EchoUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectReverbConfig(ReverbUnit *unit);
void AudioEffectPlateReverbConfig(ReverbPlateUnit *unit);
void AudioEffectPlateReverbHighcutModulaConfig(ReverbPlateUnit *unit, uint8_t channel, uint32_t sample_rate);
#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
void AudioEffectReverProbConfig(ReverbProUnit *unit,uint32_t sample_rate);
void AudioEffectReverProbParameterConfig(ReverbProUnit *unit);
#endif
void AudioEffectEQPregainConfig(EQUnit *unit);
void AudioEffectEQFilterConfig(EQUnit *unit, uint32_t sample_rate);
void AudioEffectEQFilterClearBufConfig(EQUnit *unit, uint32_t sample_rate);
void AudioEffectVocalRemoveConfig(VocalRemoveUnit *unit,  uint8_t channel, uint32_t sample_rate);
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
void AudioEffectVBConfig(VBUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
void AudioEffectVBClassicConfig(VBClassicUnit *unit, uint8_t channel, uint32_t sample_rate);
#endif
void AudioEffectHowlingSuppressorConfig(HowlingDectorUnit *unit);
void AudioEffectDRCConfig(DRCUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectPhaseApply(PhaseControlUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n,  uint8_t channel);
#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
void AudioEffectPcmDelayApply(PcmDelayUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectPcmDelayApply24(PcmDelayUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
#endif
void AudioEffectAecApply(AecUnit *unit, int16_t *u_pcm_in, int16_t *d_pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectExciterApply(ExciterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectExciterApply24(ExciterUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectExpanderApply(ExpanderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectFreqShifterApply(FreqShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectFreqShifterApply24(FreqShifterUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectFreqShifterFineApply(FreqShifterFineUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectFreqShifterFineApply24(FreqShifterFineUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectHowlingSuppressorApply(HowlingDectorUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n);
void AudioEffectHowlingSuppressorApply24(HowlingDectorUnit *unit, int32_t *pcm_in, int32_t *pcm_out, int32_t n);
void AudioEffectPregainApply(GainControlUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n, uint8_t channel);
void AudioEffectPitchShifterApply(PitchShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n, uint8_t channel);

#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
void AudioEffectPitchShifterProApply(PitchShifterProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectPitchShifterProApply24(PitchShifterProUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
#endif

void AudioEffectAutoTuneApply(AutoTuneUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectAutoTuneApply24(AutoTuneUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectVoiceChangerApply(VoiceChangerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
void AudioEffectVoiceChangerProApply(VoiceChangerProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
#endif
void AudioEffectReverbApply(ReverbUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectPlateReverbApply(ReverbPlateUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectPlateReverbApply24(ReverbPlateUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
void AudioEffectReverbProApply(ReverbProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
#endif
void AudioEffectEchoApply(EchoUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectEchoApply24(EchoUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectDRCApply(DRCUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectDRCApply24(DRCUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectEQApply(EQUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n, uint8_t channel);
void AudioEffectEQApply24(EQUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n, uint8_t channel);
void AudioEffectSilenceDectorApply(SilenceDetectorUnit *unit, int16_t *pcm_in, int32_t n);
void AudioEffectSilenceDectorApply24(SilenceDetectorUnit *unit, int32_t *pcm_in, int32_t n);
void AudioEffectVocalCutApply(VocalCutUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectVocalRemoveApply(VocalRemoveUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectChorusApply(ChorusUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectChorus2Apply(Chorus2Unit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectChorus2Apply24(Chorus2Unit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
void AudioEffectThreeDApply(ThreeDUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectThreeDApply24(ThreeDUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
#endif
#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
void AudioEffectThreeDPlusApply(ThreeDPlusUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
void AudioEffectVBApply(VBUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectVBApply24(VBUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
void AudioEffectVBClassicApply(VBClassicUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectVBClassicApply24(VBClassicUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
#endif
void AudioEffectStereoWidenerInit(StereoWindenUnit *unit, uint32_t sample_rate);
void AudioEffectStereoWidenerApply(StereoWindenUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectStereoWidenerApply24(StereoWindenUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectPinPongApply(PingPongUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectPingPongInit(PingPongUnit *unit, uint32_t sample_rate,uint8_t bit_width);

void AudioEffectAutoWahInit(AutoWahUnit *unit, uint32_t sample_rate);
void AudioEffectAutoWahApply(AutoWahUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectAutoWahApply24(AutoWahUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectFlangerInit(FlangerUnit *unit,  uint32_t sample_rate,uint16_t bit_width);
void AudioEffectFlangerApply(FlangerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectFlangerApply24(FlangerUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectOverdriveInit(OverdriveUnit *unit,  uint32_t sample_rate);
void AudioEffectOverdriveApply(OverdriveUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectDistortionDS1Apply(DistortionDS1Unit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectDistortionDS1Configer(DistortionDS1Unit *unit);
void AudioEffectDistortionDS1Init(DistortionDS1Unit *unit,  int32_t sample_rate);
void AudioEffectDistortionExpInit(DistortionUnit *unit,  uint32_t sample_rate);
void AudioEffectDistortionExpApply(DistortionUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectOverdrivePolyInit(OverdrivePolyUnit *unit,  uint32_t sample_rate);
void AudioEffectOverdrivePolyApply(OverdrivePolyUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectPitchDetectorInit(PitchDetectorUnit *unit,  uint32_t sample_rate);
void AudioEffectPitchDetectorApply(PitchDetectorUnit *unit, int16_t *pcm_in,uint32_t n);
void AudioEffectPitchDetectorParameterInit(void);

void AudioEffectEqDrcInit(EQ_DRCUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectEqDrcFilterConfig(EQ_DRCUnit *unit, uint32_t sample_rate);
void AudioEffectEqDrcFilterConfigForEq(EQ_DRCUnit *unit, uint32_t sample_rate);
void AudioEffectEqDrcApply(EQ_DRCUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectBlueNSInit(BlueNsUnit *unit,uint16_t block_size,uint16_t bit_depth);
void AudioEffectBlueNSApply(BlueNsUnit *unit, int16_t *pcm_in, int16_t *pcm_out);

void AudioEffectProcess(AudioCoreContext *pAudioCore);
void AudioMusicProcess(AudioCoreContext *pAudioCore);
void AudioBypassProcess(AudioCoreContext *pAudioCore);
void AudioEffectProcessBTRecord(AudioCoreContext *pAudioCore);
void AudioEffectProcessBTHF(AudioCoreContext *pAudioCore);
void AudioEffectProcessUsbPhone(AudioCoreContext *pAudioCore);
void AudioNoAppProcess(AudioCoreContext *pAudioCore);
void AudioEffectFaderInit(FaderUnit *unit,uint8_t channel, uint32_t fade_samples);
void AudioEffectFaderDenit(FaderUnit *unit);
void AudioEffectFaderInApply(FaderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectFaderOutApply(FaderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectFaderInApply24(FaderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectFaderOutApply24(FaderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectCompanderInit(CompanderUnit *unit,uint8_t channel, uint32_t sample_rate);
void AudioEffectCompanderApply24(CompanderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectCompanderApply(CompanderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectLowLevelCompressorInit(LowLevelCompressorUnit *unit,uint8_t channel, uint32_t sample_rate);
void AudioEffectLowLevelCompressorApply(LowLevelCompressorUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectLowLevelCompressorApply24(LowLevelCompressorUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);

void AudioEffectHowlingSuppressorFineInit(HowlingDectorFineUnit *unit,uint32_t sample_rate);
void AudioEffectHowlingSuppressorFineApply(HowlingDectorFineUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n);
void AudioEffectHowlingSuppressorFineApply24(HowlingDectorFineUnit *unit, int32_t *pcm_in, int32_t *pcm_out, int32_t n);

void AudioEffectBiquadApply24(BiquadUnit *unit, int32_t *pcm_in, int32_t *pcm_out, int32_t n);
void AudioEffectBiquadApply(BiquadUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectBiquadInit(BiquadUnit *unit,  uint16_t num_channels,uint32_t sample_rate);

void AudioEffectHowlingGuardInit(HowlingGuardUnit *unit, uint32_t sample_rate);
void AudioEffectHowlingGuardApply(HowlingGuardUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioEffectPhaseShifterdInit(PhaseShifterUnit *unit, uint32_t num_channels,uint32_t step_size,uint32_t sample_rate);
void AudioEffectPhaseShifterConfigure(PhaseShifterUnit *unit, uint32_t sample_rate);
void AudioEffectPhaseShifterApply(PhaseShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectPhaseShifterApply24(PhaseShifterUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
//------------------------------//
void Communication_Effect_Chorus2(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void Communication_Effect_HowlingGuard(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void Communication_Effect_PhaseShifter(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);

void AudioEffectDraPostApply(DraPostUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectDraPostInit(DraPostUnit *unit,uint16_t channel, uint32_t sample_rate);
void AudioEffectDraPostReset(DraPostUnit *unit);
void Communication_Effect_DraPost(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void Communication_Effect_DraPost_page1(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void Communication_Effect_DraPost_page2(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void Communication_Effect_DraPost_page3(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void Communication_Effect_DraPost_page4(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void Communication_Effect_DraPost_page5(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);

void AudioEffectDCBlockerInit(DCBlockerUnit *unit,uint8_t channel);
void AudioEffectDCBlockerApply(DCBlockerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectDCBlockerApply24(DCBlockerUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void Communication_Effect_DCBlocker(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);

void AudioEffectVirtualSurroundApply24(VirtualSurroundUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectVirtualSurroundApply(VirtualSurroundUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectVirtualSurroundInit(VirtualSurroundUnit *unit,uint8_t channel,uint16_t SampleRate);
void Communication_Effect_VBSurround(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);

void AudioEffectNoiseGeneratorApply24(NoiseGeneratorUnit *unit, int32_t *pcm_out, uint32_t n);
void AudioEffectNoiseGeneratordApply(NoiseGeneratorUnit *unit, int16_t *pcm_out, uint32_t n);
void AudioEffectNoiseGeneratorInit(NoiseGeneratorUnit *unit,uint8_t channel,uint16_t SampleRate);

void AudioEffectButterWorthApply(ButterWorthUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectButterWorthApply24(ButterWorthUnit *unit, int32_t *pcm_in, int32_t *pcm_out, int32_t n);
void AudioEffectButterWorthInit(ButterWorthUnit *unit, uint32_t num_channels,uint32_t sample_rate);
void Communication_Effect_ButterWorth(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);

void AudioEffectDynamicEqApply(DynamicEqUnit *unit, int16_t *pcm_in, int16_t *pcm_out,uint32_t n);
void AudioEffectDynamicEqApply24(DynamicEqUnit *unit, int32_t *pcm_in, int32_t *pcm_out,int32_t n);
void AudioEffectDynamicEqInit(DynamicEqUnit *unit, uint32_t num_channels,uint32_t sample_rate);
void Communication_Effect_DynamicEQ(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void Communication_Effect_LRBalancer(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void AudioEffectLRBalancerInit(LRBalancerUnit *unit,uint8_t channel);
void AudioEffectLRBalancerApply(LRBalancerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectLRBalancerApply24(LRBalancerUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n);
void AudioEffectHowlingSuppressorSpecifiedApply(HowlingSpecifiedUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n);
void AudioEffectHowlingSuppressorSpecifieInit(HowlingSpecifiedUnit *unit,uint32_t sample_rate);
void Communication_Effect_HowlingSpecified(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);
void Communication_Effect_HowlingSpecified_1(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len);

void AudioEffectVadInit(VADUnit *unit,uint32_t sample_rate,uint16_t SamplesPerFrame);
void AudioEffectVadApply(VADUnit *unit, int16_t *pcm_in,  uint32_t n);

void AudioEffectDRCLegacyInit(DRCLegacyUnit *unit, uint32_t num_channels,uint32_t sample_rate);
void AudioEffectDRCLegacyConfig(DRCLegacyUnit *unit, uint8_t channel, uint32_t sample_rate);
void AudioEffectDRCLegacyApply(DRCLegacyUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);
void AudioEffectDRCLegacyApply24(DRCLegacyUnit *unit, int32_t *pcm_in,int32_t *pcm_out, int32_t n);

void AudioEffectHowlingGuardConfigure(HowlingGuardUnit *unit, uint32_t sample_rate);

//#ifdef CFG_FUNC_TSM_EN
void AudioEffectTsmConfigure(int32_t speed_ratio);
void AudioEffectTsmInit(uint32_t channel, uint32_t sample_rate);
void DecoderTsmProcess(void);
void AudioEffectTsmApply(int16_t *pcm_in, int16_t *pcm_out);
//-------------------------------------------//
void AudioProcess(void *Buf);
void AudioOutProcess(void);
void AudioEffectsInit(void);
void AudioEffectsDeInit(void);
void AudioEffectsAllDisable(void);
void AudioEffectModeSel(uint16_t mode, uint8_t init_flag);
void GetAudioEffectMaxValue(void);
void ReverbStepSet(uint8_t ReverbStep);
void Communication_Effect_SetPcSta(uint8_t Control, uint8_t *buf);
void Communication_Effect_GetPcSta(uint8_t *buf, uint32_t len);
void Communication_Effect_IsUpdataToPC(void);
void UART1_Communication_Init(uint8_t *s_tx_buf, uint32_t s_tx_len, uint8_t *s_rx_buf, uint32_t s_rx_len);
#ifdef CFG_FUNC_MIC_TREB_BASS_EN
void MicBassTrebAjust(int16_t 	BassGain,	int16_t TrebGain);
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
void MusicBassTrebAjust(int16_t 	BassGain,	int16_t TrebGain);
#endif
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
void LoadEqMode(const uint8_t *buff);
#endif
#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__CTRLVARS_H__

