#!/bin/bash

# MySQL and Python interfaces
sudo apt-get install -y libmysqlclient-dev python-mysqldb
easy_install MySQL-python

# Scientific python and matplotlib
sudo apt-get install -y libblas-dev liblapack-dev python-dev libatlas-base-dev gfortran python-setuptools python-scipy python-matplotlib

# Full LAMP server (not really necessary, just MySQL part so far)
sudo apt-get install -y apache2  php5 mysql-client mysql-server vsftpd

# Create the actual database and associated tables
# TODO: replace this with the real schema
DB_USER=`whoami`
mysql -u root -e "create user '$DB_USER'@'localhost' ; grant all privileges on *.* to '$DB_USER'@'localhost'; \
create database Pc3; use Pc3; create table Data (X Integer, Y Integer)' -p