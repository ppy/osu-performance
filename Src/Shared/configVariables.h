#ifndef MACRO_CONFIG_INT
#define MACRO_CONFIG_INT( name, def, min, max, desc )
#endif

#ifndef MACRO_CONFIG_FLOAT
#define MACRO_CONFIG_FLOAT( name, def, min, max, desc )
#endif

#ifndef MACRO_CONFIG_STR
#define MACRO_CONFIG_STR( name, def, maxlen, desc )
#endif

MACRO_CONFIG_STR(MySQL_db_host, "db", 100, "MySQL db host")
MACRO_CONFIG_INT(MySQL_db_port, 3306, 0, 100000, "MySQL db port")
MACRO_CONFIG_STR(MySQL_db_username, "performance", 100, "MySQL db username")
MACRO_CONFIG_STR(MySQL_db_password, "", 100, "MySQL db password")
MACRO_CONFIG_STR(MySQL_db_database, "osu", 100, "MySQL db database")

MACRO_CONFIG_STR(MySQL_db_slave_host, "db_slave", 100, "MySQL db_slave host")
MACRO_CONFIG_INT(MySQL_db_slave_port, 3306, 0, 100000, "MySQL db_slave port")
MACRO_CONFIG_STR(MySQL_db_slave_username, "performance", 100, "MySQL db_slave username")
MACRO_CONFIG_STR(MySQL_db_slave_password, "", 100, "MySQL db_slave password")
MACRO_CONFIG_STR(MySQL_db_slave_database, "osu", 100, "MySQL db_slave database")

MACRO_CONFIG_INT(DifficultyUpdateInterval, 10000, 1, 1000000000, "Beatmap update interval in milliseconds")
MACRO_CONFIG_INT(ScoreUpdateInterval, 50, 1, 1000000000, "Score update interval in milliseconds")

MACRO_CONFIG_INT(StallTimeThreshold, 600000, 1, 1000000000, "Time in milliseconds after which an emergency shut down is performed if no beatmap updates occured")

MACRO_CONFIG_STR(UserPPColumnName, "rank_score", 100, "Column name of the user's pp value in osu_users")
MACRO_CONFIG_STR(UserMetadataTableName, "sample_users", 100, "Name of the table containing user metadata such as username and restriction status")

MACRO_CONFIG_STR(SlackHookDomain, "", 100, "Slack domain")
MACRO_CONFIG_STR(SlackHookKey, "", 100, "API key for authentication")
MACRO_CONFIG_STR(SlackHookChannel, "", 100, "Channel to send messages into")
MACRO_CONFIG_STR(SlackHookUsername, "", 100, "Username of this pp processor")
MACRO_CONFIG_STR(SlackHookIconURL, "", 100, "Avatar image URL for this pp processor")

MACRO_CONFIG_STR(SentryDomain, "", 100, "Sentry domain")
MACRO_CONFIG_INT(SentryProjectID, 0, 0, 1000, "Project ID")
MACRO_CONFIG_STR(SentryPublicKey, "", 100, "Public key for authentication")
MACRO_CONFIG_STR(SentryPrivateKey, "", 100, "Private key for authentication")

#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_FLOAT
#undef MACRO_CONFIG_STR
