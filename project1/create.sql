CREATE DATABASE Pokemon CHARACTER SET = utf8;

USE Pokemon;

CREATE TABLE Pokemon(
	id INT PRIMARY KEY COMMENT 'id',
	name VARCHAR(20) COMMENT 'name',
	type VARCHAR(20) COMMENT 'type'
);

CREATE TABLE Evolution(
	before_id INT COMMENT 'before_id' REFERENCES Pokemon (id),
	after_id INT COMMENT 'after_id' REFERENCES Pokemon (id)
);

CREATE TABLE City(
	name VARCHAR(20) PRIMARY KEY COMMENT 'name',
	description VARCHAR(100) COMMENT 'description'
);

CREATE TABLE Trainer(
	id INT AUTO_INCREMENT PRIMARY KEY COMMENT 'id',
	name VARCHAR(20) COMMENT 'name',
	hometown VARCHAR(100) COMMENT 'hometown' REFERENCES City (description)
);

CREATE TABLE Gym(
	leader_id INT COMMENT 'leader_id'  REFERENCES Trainer (id),
	city VARCHAR(20) COMMENT 'city' REFERENCES City (name)
);

CREATE TABLE CatchedPokemon(
	id INT AUTO_INCREMENT PRIMARY KEY COMMENT 'id',
	owner_id INT COMMENT 'owner_id' REFERENCES Trainer (id),
	pid INT COMMENT 'pid' REFERENCES Pokemon (id),
	level INT COMMENT 'level',
	nickname VARCHAR(100) COMMENT 'nickname'
);