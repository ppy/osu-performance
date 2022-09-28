#!/bin/bash

set -e

if [ ! -f config.json ]; then
  if [[ -v MYSQL_HOST ]] && [[ -v MYSQL_USER ]] && [[ -v MYSQL_PASSWORD ]] && [[ -v MYSQL_DATABASE ]]; then
    TEMPLATE='{
      "mysql.master.host": env.MYSQL_HOST,
      "mysql.master.port": (env.MYSQL_PORT // 3306) | tonumber,
      "mysql.master.username": env.MYSQL_USER,
      "mysql.master.password": env.MYSQL_PASSWORD,
      "mysql.master.database": env.MYSQL_DATABASE,'

    if [[ -v MYSQL_SLAVE_HOST ]] && [[ -v MYSQL_SLAVE_USER ]] && [[ -v MYSQL_SLAVE_PASSWORD ]] && [[ -v MYSQL_SLAVE_DATABASE ]]; then
      TEMPLATE+='
        "mysql.slave.host": env.MYSQL_SLAVE_HOST,
        "mysql.slave.port": (env.MYSQL_SLAVE_PORT // 3306) | tonumber,
        "mysql.slave.username": env.MYSQL_SLAVE_USER,
        "mysql.slave.password": env.MYSQL_SLAVE_PASSWORD,
        "mysql.slave.database": env.MYSQL_SLAVE_DATABASE,'
    fi
    if [[ -v MYSQL_USER_PP_TABLE_NAME ]]; then
      TEMPLATE+='
        "mysql.user-pp-column-name": env.MYSQL_USER_PP_TABLE_NAME,'
    fi
    if [[ -v MYSQL_USER_METADATA_TABLE_NAME ]]; then
      TEMPLATE+='
        "mysql.user-metadata-table-name": env.MYSQL_USER_METADATA_TABLE_NAME,'
    fi
    if [[ -v WRITE_ALL_PP ]]; then
      TEMPLATE+='
        "write-all-pp": ((env.WRITE_ALL_PP | ascii_downcase) == "true" or env.WRITE_ALL_PP == "1"),'
    fi
    if [[ -v POLL_INTERVAL_DIFFICULTIES ]]; then
      TEMPLATE+='
        "poll.interval.difficulties": env.POLL_INTERVAL_DIFFICULTIES | tonumber,'
    fi
    if [[ -v POLL_INTERVAL_SCORES ]]; then
      TEMPLATE+='
        "poll.interval.scores": env.POLL_INTERVAL_SCORES | tonumber,'
    fi
    if [[ -v SENTRY_HOST ]] && [[ -v SENTRY_PROJECTID ]] && [[ -v SENTRY_PUBLICKEY ]] && [[ -v SENTRY_PRIVATEKEY ]]; then
      TEMPLATE+='
        "sentry.host": env.SENTRY_HOST,
        "sentry.project-id": env.SENTRY_PROJECTID | tonumber,
        "sentry.public-key": env.SENTRY_PUBLICKEY,
        "sentry.private-key": env.SENTRY_PRIVATEKEY,'
    fi
    if [[ -v DATADOG_HOST ]]; then
      TEMPLATE+='
        "data-dog.host": env.DATADOG_HOST,'
    fi
    if [[ -v DATADOG_PORT ]]; then
      TEMPLATE+='
        "data-dog.port": (env.DATADOG_PORT // 8125) | tonumber,'
    fi

    TEMPLATE+="}"
    jq -n "$TEMPLATE" > config.json
  else
    echo "Please provide the /srv/config.json file or define at least MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD and MYSQL_DATABASE!"
    exit 1
  fi
fi

./osu-performance "$@"
