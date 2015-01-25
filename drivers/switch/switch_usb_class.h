
#define FSA9685_OPEN                       0
#define FSA9685_USB1_ID_TO_IDBYPASS        1
#define FSA9685_USB2_ID_TO_IDBYPASS        2
#define FSA9685_MHL_ID_TO_CBUS             4

struct switch_usb_info {
    struct atomic_notifier_head charger_type_notifier_head;
    spinlock_t reg_flag_lock;
};

extern int fsa9685_manual_sw(int input_select);
extern int fsa9685_manual_detach(void);
