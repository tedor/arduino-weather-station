CREATE USER weather WITH PASSWORD '123456';

CREATE DATABASE weather
  WITH OWNER = weather
       ENCODING = 'UTF8'
       TABLESPACE = pg_default
       LC_COLLATE = 'C'
       LC_CTYPE = 'C'
       CONNECTION LIMIT = -1;


CREATE TABLE sensor
(
  sensor_id serial NOT NULL,
  device_id integer,
  temperature numeric(3,1),
  pressure integer,
  humidity numeric(3,1),
  battery_voltage numeric(3,2),
  created_at timestamp with time zone
)
WITH (
  OIDS=FALSE
);
ALTER TABLE sensor
  OWNER TO weather;