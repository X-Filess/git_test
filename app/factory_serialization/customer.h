
//#define CUSTOMER_1_SUPPORT
#define CUSTOMER_2_SUPPORT

int get_card_id(char *buffer, unsigned int bytes);

int set_card_id(char *buffer, unsigned int bytes);

int get_provider_id(char *buffer, unsigned int bytes);

int set_provider_id(char *buffer, unsigned int bytes);

int get_dsn_id(char *buffer, unsigned int bytes);

int get_chip_id(char *buffer, unsigned int bytes);

int lock_otp(void);

int get_factory_serialization_flag(void);
