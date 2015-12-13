// Input Booster +
#ifndef _INPUT_BOOSTER_H_
#define _INPUT_BOOSTER_H_

#include <linux/pm_qos.h>
#include <linux/of.h>

#ifdef CONFIG_SCHED_HMP
#define USE_HMP_BOOST
#endif

#define MAX_EVENTS   			15
#define MAX_MULTI_TOUCH_EVENTS   	5

#define set_qos(req, pm_qos_class, value) { \
	if (value) { \
		if (pm_qos_request_active(req)) {\
			pr_debug("[Input Booster] ******      pm_qos_update_request : %d\n", value); \
			pm_qos_update_request(req, value); \
		} else { \
			pr_debug("[Input Booster] ****        pm_qos_add_request : %d\n", value); \
			pm_qos_add_request(req, pm_qos_class, value); \
		} \
	} else { \
		remove_qos(req); \
	}\
}

#define remove_qos(req) { \
	if (pm_qos_request_active(req)) \
		pm_qos_remove_request(req); \
}

#ifdef USE_HMP_BOOST
#define set_hmp(enable)	 { \
	if(enable != current_hmp_boost) { \
		if (set_hmp_boost(enable) < 0) {\
			pr_debug("[Input Booster] ******            !!! fail to HMP !!!\n"); \
		} \
		current_hmp_boost = enable; \
	} \
}
#else
#define set_hmp(enable)
#endif

#ifdef CONFIG_ARCH_EXYNOS //______________________________________________________________________________
#define SET_BOOSTER  { \
	set_hmp(_this->param[_this->index].hmp_boost); \
	set_qos(&_this->cpu_qos, PM_QOS_CPU_FREQ_MIN, _this->param[_this->index].cpu_freq);  \
	set_qos(&_this->kfc_qos, PM_QOS_KFC_FREQ_MIN, _this->param[_this->index].kfc_freq);  \
	set_qos(&_this->mif_qos, PM_QOS_BUS_THROUGHPUT, _this->param[_this->index].mif_freq);  \
	set_qos(&_this->int_qos, PM_QOS_DEVICE_THROUGHPUT, _this->param[_this->index].int_freq);  \
}
#define REMOVE_BOOSTER  { \
	set_hmp(0);  \
	remove_qos(&_this->cpu_qos);  \
	remove_qos(&_this->kfc_qos);  \
	remove_qos(&_this->mif_qos);  \
	remove_qos(&_this->int_qos);  \
}
#define PROPERTY_BOOSTER(_device_param_, _dt_param_)  { \
	_device_param_->cpu_freq = _dt_param_->cpu_freq; \
	_device_param_->kfc_freq = _dt_param_->kfc_freq; \
	_device_param_->mif_freq = _dt_param_->mif_freq; \
	_device_param_->int_freq = _dt_param_->int_freq; \
	_device_param_->time = _dt_param_->head_time; \
	_device_param_->hmp_boost = _dt_param_->hmp_boost; \
	pr_debug("[Input Booster] device_type:%d, label :%s, type: 0x%02x, num_level[%d]\n",  \
		device_tree_infor[i].type, device_tree_infor[i].label, device_tree_infor[i].type, device_tree_infor[i].nlevels);  \
	pr_debug("[Input Booster] Level %d : frequency[%d,%d,%d,%d] hmp_boost[%d] times[%d,%d,%d]\n", i, \
		_dt_param_->cpu_freq, _dt_param_->kfc_freq, _dt_param_->mif_freq, _dt_param_->int_freq, \
		_dt_param_->hmp_boost, _dt_param_->head_time, _dt_param_->tail_time, _dt_param_->phase_time); \
}
#endif //______________________________________________________________________________

#define INIT_BOOSTER(_device_) { \
	_device_##_booster.input_booster_state = input_booster_idle_state; \
	INIT_DELAYED_WORK(&_device_##_booster.input_booster_timeout_work, TIMEOUT_FUNC(_device_)); \
	INIT_WORK(&_device_##_booster.input_booster_set_booster_work, SET_BOOSTER_FUNC(_device_)); \
	mutex_init(&_device_##_booster.lock); \
	{ \
		int i; \
		for(i=0;i<ndevice_in_dt;i++) { \
			if(device_tree_infor[i].type == _device_##_booster_dt.type) { \
				struct t_input_booster_param *device_param = &_device_##_booster.param[0]; \
				struct t_input_booster_param *device_param2 = &_device_##_booster.param[1]; \
				struct t_input_booster_device_tree_param *dt_param = &device_tree_infor[i].param_tables[_device_##_booster_dt.head_level - 1]; \
				struct t_input_booster_device_tree_param *dt_param2 = &device_tree_infor[i].param_tables[_device_##_booster_dt.tail_level - 1]; \
				if(device_param != NULL && dt_param != NULL) { \
					PROPERTY_BOOSTER(device_param, dt_param) \
				} \
				if(device_param2 != NULL && dt_param2 != NULL) { \
					PROPERTY_BOOSTER(device_param2, dt_param2) \
				} \
				break; \
			} \
		} \
	} \
}

#define TIMEOUT_FUNC(_DEVICE_) input_booster_##_DEVICE_##_timeout_work_func

#define DECLARE_TIMEOUT_FUNC(_DEVICE_) \
static void input_booster_##_DEVICE_##_timeout_work_func(struct work_struct *work)  \
{ \
	struct t_input_booster *_this = &_DEVICE_##_booster; \
	int param_max = sizeof(_this->param)/sizeof(struct t_input_booster_param);  \
	mutex_lock(&_this->lock); \
	if(_this->index < param_max && _this->param[_this->index].time > 0) { \
		pr_debug("[Input Booster] ******           Timeout : changed  index : %d, time : %d\n", _this->index, _this->param[_this->index].time); \
		pr_debug("[Input Booster] ****             hmp : %d  cpu : %d\n", _this->param[_this->index].hmp_boost, _this->param[_this->index].cpu_freq); \
		SET_BOOSTER; \
	} else { \
		pr_debug("[Input Booster] Timeout : completed\n"); \
		pr_debug("[Input Booster]\n"); \
		REMOVE_BOOSTER; \
		_this->index = 0; \
		CHANGE_STATE_TO(idle); \
	} \
	mutex_unlock(&_this->lock); \
}

#define SET_BOOSTER_FUNC(_DEVICE_) input_booster_##_DEVICE_##_set_booster_work_func

#define DECLARE_SET_BOOSTER_FUNC(_DEVICE_) \
static void input_booster_##_DEVICE_##_set_booster_work_func(struct work_struct *work)  \
{ \
	struct t_input_booster *_this = (struct t_input_booster *)(&_DEVICE_##_booster); \
	mutex_lock(&_this->lock); \
	SET_BOOSTER; \
	_this->index++; \
	mutex_unlock(&_this->lock); \
}

#define DECLARE_STATE_FUNC(_STATE_) void input_booster_##_STATE_##_state(void *__this, int input_booster_event)

#define CHANGE_STATE_TO(_STATE_) _this->input_booster_state = input_booster_##_STATE_##_state;	


enum booster_mode_on_off {
	BOOSTER_OFF = 0,
	BOOSTER_ON,
};


struct input_value input_events[MAX_EVENTS+1];

struct t_input_booster_param {
	u32 cpu_freq;
	u32 kfc_freq;
	u32 mif_freq;
	u32 int_freq;

	u16 time;

	u8 hmp_boost;
	u8 dummy;
};

struct t_input_booster {
	struct mutex lock;
	struct t_input_booster_param param[2];

	struct pm_qos_request	cpu_qos;
	struct pm_qos_request	kfc_qos;
	struct pm_qos_request	mif_qos;
	struct pm_qos_request	int_qos;

	struct delayed_work 	input_booster_timeout_work;
	struct work_struct 	input_booster_set_booster_work;

	int index;
	int multi_events;

	void (*input_booster_state)(void *__this, int input_booster_event);
};

//+++++++++++++++++++++++++++++++++++++++++++++++  STRUCT & VARIABLE FOR DEVICE TREE  +++++++++++++++++++++++++++++++++++++++++++++++//
struct t_input_booster_device_tree_param {
	u8 	ilevels;

	u8 	hmp_boost;

	u16 	head_time;
	u16 	tail_time;
	u16 	phase_time;

	u32 	cpu_freq;
	u32 	kfc_freq;
	u32 	mif_freq;
	u32 	int_freq;
};

struct t_input_booster_device_tree_infor {
	const char 	*label;
	int 	type;
	int 	nlevels;

	struct t_input_booster_device_tree_param 	*param_tables;
};

struct t_input_booster_device_tree_gender {
	int type;
	int head_level;
	int tail_level;
};

//______________________________________________________________________________	<<< in DTSI file >>>
//______________________________________________________________________________	input_booster,type = <4>;	/* BOOSTER_DEVICE_KEYBOARD */
//______________________________________________________________________________	input_booster,levels = <1 2>;
struct t_input_booster_device_tree_gender 	keyboard_booster_dt = {4,1,2}; 		// type : 4, head level : 1, tail level 2
struct t_input_booster_device_tree_gender 	mouse_booster_dt = {5,1,2};		// type : 5, head level : 1, tail level 2
struct t_input_booster_device_tree_gender 	mouse_wheel_booster_dt = {6,1,2};	// type : 6, head level : 1, tail level 2
struct t_input_booster_device_tree_infor 	*device_tree_infor = NULL;

int ndevice_in_dt;
//----------------------------------------------  STRUCT & VARIABLE FOR DEVICE TREE  ----------------------------------------------//

int TouchIDs[MAX_MULTI_TOUCH_EVENTS];
bool current_hmp_boost = 0;

struct t_input_booster	keyboard_booster;
struct t_input_booster	mouse_booster;
struct t_input_booster	mouse_wheel_booster;

int input_count = 0;

void input_booster_idle_state(void *__this, int input_booster_event);
void input_booster_press_state(void *__this, int input_booster_event);
void input_booster(struct input_dev *dev);
void input_booster_init(void);
#endif
// Input Booster -
