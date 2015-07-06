CREATE TABLE measurement1
(
  serid integer NOT NULL,
  measurement_ts timestamp without time zone NOT NULL DEFAULT now(),
  measurement_value double precision,
  CONSTRAINT measurement1_pkey PRIMARY KEY (measurement_ts, serid)
)
WITH (
  OIDS=FALSE
);
ALTER TABLE measurement1 OWNER TO monlink;

create table quantity(
	qtyid integer not null primary key,
	qtycode varchar(32) not null,
	qtylabel varchar(100) not null,
	qtyunit varchar(100),
	qtydesc varchar(255)
);

create table sensor (
	serid	integer not null primary key,
	qtyid	integer not null,
	remark	varchar(255),
	foreign key(qtyid) references quantity(qtyid)
);

insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(1, 'DUR', 'Total duration in second(s)', 's', 'Measurement duration since power on');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(2, 'HOUR', 'Duration in hour(s)', 'h', 'Measurement duration since power on');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(3, 'MINUTE', 'Duration in minute(s)', 'm', 'Measurement duration since power on');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(4, 'SECOND', 'Duration in second(s)', 's', 'Measurement duration since power on');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(5, 'COUNT', 'Pulse count', '', 'Number of pulse between sampling interval');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(6, 'CPS', 'Count per seconds', 'cps', 'Dose rate in cps');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(7, 'USVS', 'Dose rate', 'uSv/h', 'Instant dose rate in uSv/h');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(8, 'USVSE', 'Dose rate error', 'uSv/h', 'Instant dose rate error');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(9, 'CPM', 'Count per minute', 'cpm', 'Dose rate in cpm');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(10, 'USVM', 'Dose rate', 'uSv/h', 'Dose rate in uSv/h');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(11, 'USVME', 'Dose rate error', 'uSv/h', 'Dose rate error');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(12, 'PIR', 'Presence', '', 'Human presence sensor');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(13, 'TEMP', 'Temperature', 'C', 'Temperature in celcius');
insert into quantity(qtyid, qtycode, qtylabel, qtyunit, qtydesc) values(14, 'HUM', 'Humidity', '%', 'Relative humidity');

insert into sensor(serid, qtyid, remark) values(1, 1, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(2, 2, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(3, 3, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(4, 4, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(5, 5, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(6, 6, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(7, 7, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(8, 8, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(9, 9, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(10, 10, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(11, 11, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(12, 12, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(13, 13, 'Pocket Geiger WiFi');
insert into sensor(serid, qtyid, remark) values(14, 14, 'Pocket Geiger WiFi');

