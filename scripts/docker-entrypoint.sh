#!/bin/bash

if [ ! -f config.json ]; then
  if [[ -v MYSQL_HOST ]] && [[ -v MYSQL_USER ]] && [[ -v MYSQL_PASSWORD ]] && [[ -v MYSQL_DATABASE ]]; then
    jq -n \
      --arg MYSQL_HOST "$MYSQL_HOST" \
      --arg MYSQL_PORT "${MYSQL_PORT:-3306}" \
      --arg MYSQL_USER "$MYSQL_USER" \
      --arg MYSQL_PASSWORD "$MYSQL_PASSWORD" \
      --arg MYSQL_DATABASE "$MYSQL_DATABASE" \
      '{
        "mysql.master.host": $MYSQL_HOST,
        "mysql.master.port" : $MYSQL_PORT | tonumber,
        "mysql.master.username" : $MYSQL_USER,
        "mysql.master.password" : $MYSQL_PASSWORD,
        "mysql.master.database" : $MYSQL_DATABASE
      }' > config.json
  else
    echo "Please provide the /srv/config.json file or define MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD and MYSQL_DATABASE!"
    exit 1
  fi
fi

./osu-performance $@
