-- MySQL dump 10.13  Distrib 5.7.27, for Linux (x86_64)
--
-- Host: localhost    Database: mysql_innodb_cluster_metadata
-- ------------------------------------------------------
-- Server version	5.7.27-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Current Database: `mysql_innodb_cluster_metadata`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `mysql_innodb_cluster_metadata` /*!40100 DEFAULT CHARACTER SET latin1 */;

USE `mysql_innodb_cluster_metadata`;

--
-- Table structure for table `clusters`
--

DROP TABLE IF EXISTS `clusters`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `clusters` (
  `cluster_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `cluster_name` varchar(40) NOT NULL,
  `default_replicaset` int(10) unsigned DEFAULT NULL,
  `description` text,
  `mysql_user_accounts` blob,
  `options` json DEFAULT NULL,
  `attributes` json DEFAULT NULL,
  PRIMARY KEY (`cluster_id`),
  UNIQUE KEY `cluster_name` (`cluster_name`),
  KEY `default_replicaset` (`default_replicaset`),
  CONSTRAINT `clusters_ibfk_1` FOREIGN KEY (`default_replicaset`) REFERENCES `replicasets` (`replicaset_id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb4;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `clusters`
--

LOCK TABLES `clusters` WRITE;
/*!40000 ALTER TABLE `clusters` DISABLE KEYS */;
INSERT INTO `clusters` VALUES (1,'sample',1,'Default Cluster',NULL,NULL,'{\"default\": true, \"opt_disableClone\": true, \"opt_gtidSetIsComplete\": false}');
/*!40000 ALTER TABLE `clusters` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `hosts`
--

DROP TABLE IF EXISTS `hosts`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `hosts` (
  `host_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `host_name` varchar(256) DEFAULT NULL,
  `ip_address` varchar(45) DEFAULT NULL,
  `public_ip_address` varchar(45) DEFAULT NULL,
  `location` varchar(256) NOT NULL,
  `attributes` json DEFAULT NULL,
  `admin_user_account` json DEFAULT NULL,
  PRIMARY KEY (`host_id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb4;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `hosts`
--

LOCK TABLES `hosts` WRITE;
/*!40000 ALTER TABLE `hosts` DISABLE KEYS */;
INSERT INTO `hosts` VALUES (1,'192.168.1.108','',NULL,'',NULL,NULL),(2,'rennox-tc',NULL,NULL,'','{\"registeredFrom\": \"mysql-router\"}',NULL);
/*!40000 ALTER TABLE `hosts` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `instances`
--

DROP TABLE IF EXISTS `instances`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `instances` (
  `instance_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `host_id` int(10) unsigned NOT NULL,
  `replicaset_id` int(10) unsigned DEFAULT NULL,
  `mysql_server_uuid` varchar(40) NOT NULL,
  `instance_name` varchar(256) NOT NULL,
  `role` enum('HA','readScaleOut') NOT NULL,
  `weight` float DEFAULT NULL,
  `addresses` json NOT NULL,
  `attributes` json DEFAULT NULL,
  `version_token` int(10) unsigned DEFAULT NULL,
  `description` text,
  PRIMARY KEY (`instance_id`),
  UNIQUE KEY `mysql_server_uuid` (`mysql_server_uuid`),
  UNIQUE KEY `instance_name` (`instance_name`),
  KEY `host_id` (`host_id`),
  KEY `replicaset_id` (`replicaset_id`),
  CONSTRAINT `instances_ibfk_1` FOREIGN KEY (`host_id`) REFERENCES `hosts` (`host_id`),
  CONSTRAINT `instances_ibfk_2` FOREIGN KEY (`replicaset_id`) REFERENCES `replicasets` (`replicaset_id`) ON DELETE SET NULL
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb4;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `instances`
--

LOCK TABLES `instances` WRITE;
/*!40000 ALTER TABLE `instances` DISABLE KEYS */;
INSERT INTO `instances` VALUES (1,1,1,'2d2c3962-b072-11e9-a287-1c666d9935bf','192.168.1.108:3317','HA',NULL,'{\"mysqlX\": \"192.168.1.108:33170\", \"grLocal\": \"192.168.1.108:33171\", \"mysqlClassic\": \"192.168.1.108:3317\"}','{\"recoveryAccountHost\": \"%\", \"recoveryAccountUser\": \"mysql_innodb_cluster_15662\"}',NULL,NULL);
/*!40000 ALTER TABLE `instances` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `replicasets`
--

DROP TABLE IF EXISTS `replicasets`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `replicasets` (
  `replicaset_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `cluster_id` int(10) unsigned NOT NULL,
  `replicaset_type` enum('gr') NOT NULL,
  `topology_type` enum('pm','mm') NOT NULL DEFAULT 'pm',
  `replicaset_name` varchar(40) NOT NULL,
  `active` tinyint(1) NOT NULL,
  `attributes` json DEFAULT NULL,
  `description` text,
  PRIMARY KEY (`replicaset_id`),
  KEY `cluster_id` (`cluster_id`),
  CONSTRAINT `replicasets_ibfk_1` FOREIGN KEY (`cluster_id`) REFERENCES `clusters` (`cluster_id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb4;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `replicasets`
--

LOCK TABLES `replicasets` WRITE;
/*!40000 ALTER TABLE `replicasets` DISABLE KEYS */;
INSERT INTO `replicasets` VALUES (1,1,'gr','pm','default',1,'{\"adopted\": \"false\", \"group_replication_group_name\": \"2dd3eff5-b072-11e9-a287-1c666d9935bf\"}',NULL);
/*!40000 ALTER TABLE `replicasets` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `routers`
--

DROP TABLE IF EXISTS `routers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `routers` (
  `router_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `router_name` varchar(256) NOT NULL,
  `host_id` int(10) unsigned NOT NULL,
  `attributes` json DEFAULT NULL,
  PRIMARY KEY (`router_id`),
  UNIQUE KEY `host_id` (`host_id`,`router_name`),
  CONSTRAINT `routers_ibfk_1` FOREIGN KEY (`host_id`) REFERENCES `hosts` (`host_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `routers`
--

LOCK TABLES `routers` WRITE;
/*!40000 ALTER TABLE `routers` DISABLE KEYS */;
INSERT INTO `routers` VALUES (1,'',2,NULL);
/*!40000 ALTER TABLE `routers` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Temporary table structure for view `schema_version`
--

DROP TABLE IF EXISTS `schema_version`;
/*!50001 DROP VIEW IF EXISTS `schema_version`*/;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
/*!50001 CREATE VIEW `schema_version` AS SELECT 
 1 AS `major`,
 0 AS `minor`,
 1 AS `patch`*/;
SET character_set_client = @saved_cs_client;

--
-- Current Database: `mysql_innodb_cluster_metadata`
--

USE `mysql_innodb_cluster_metadata`;

--
-- Final view structure for view `schema_version`
--

/*!50001 DROP VIEW IF EXISTS `schema_version`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = latin1 */;
/*!50001 SET character_set_results     = latin1 */;
/*!50001 SET collation_connection      = latin1_swedish_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED */
/*!50013 DEFINER=`root`@`localhost` SQL SECURITY DEFINER */
/*!50001 VIEW `schema_version` AS select 1 AS `major`,0 AS `minor`,1 AS `patch` */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2019-07-27  8:26:48
