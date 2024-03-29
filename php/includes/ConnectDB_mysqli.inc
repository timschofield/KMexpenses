<?php

/* PeterMoulding.com
20071102 Change from mysql to mysqli;
20071102 Add $db to DB_escape_string();
*/

define('LIKE', 'LIKE');

if (!isset($DBPort)) {
	$DBPort = 3306;
}
global $db; // Make sure it IS global, regardless of our context

if (!isset($db)) {
	$db = DB_connect($Host, $DBUser, $DBPassword);

	//this statement sets the charset to be used for sending data to and from the db server
	//if not set, both mysqli server and mysqli client/library may assume otherwise
	mysqli_set_charset($db, 'utf8');
	DB_set_timezone();
}

function DB_connect($Host, $DBUser, $DBPassword) {
	return mysqli_connect('p:'.$Host, $DBUser, $DBPassword);
}

function DB_query($SQL, $ErrorMessage = '', $DebugMessage = '', $Transaction = false, $TrapErrors = false) {

	global $db;
	global $Debug;
	global $PathPrefix;
	global $RootPath;
	global $Messages;

	$Result = mysqli_query($db, $SQL);

	$_SESSION['LastInsertId'] = mysqli_insert_id($db);

	if ($DebugMessage == '') {
		$DebugMessage = _('The SQL that failed was');
	}

	if (DB_error_no() != 0 and $TrapErrors == true) {
		if ($TrapErrors) {
			require_once($PathPrefix . 'includes/header.inc');
		}
		prnMsg($ErrorMessage . '<br />' . DB_error_msg(), 'error', _('Database Error') . ' ' . DB_error_no());
		if ($Debug == 1) {
			prnMsg($DebugMessage . '<br />' . $SQL . '<br />', 'error', _('Database SQL Failure'));
		}
		if ($Transaction) {
			$SQL = 'rollback';
			$Result = DB_query($SQL);
			if (DB_error_no() != 0) {
				prnMsg(_('Error Rolling Back Transaction'), 'error', _('Database Rollback Error') . ' ' . DB_error_no());
			} else {
				prnMsg(_('Rolling Back Transaction OK'), 'error', _('Database Rollback Due to Error Above'));
			}
		}
		if ($TrapErrors) {
			include($PathPrefix . 'includes/footer.inc');
			exit;
		}
	} elseif (isset($_SESSION['MonthsAuditTrail']) and (DB_error_no() == 0 and $_SESSION['MonthsAuditTrail'] > 0) and (DB_affected_rows($Result) > 0)) {

		$SQLArray = explode(' ', $SQL);

		if (($SQLArray[0] == 'INSERT') or ($SQLArray[0] == 'UPDATE') or ($SQLArray[0] == 'DELETE')) {

			if ($SQLArray[2] != 'audittrail') { // to ensure the auto delete of audit trail history is not logged
				$AuditSQL = "INSERT INTO audittrail (transactiondate,
													userid,
													address,
													querystring)
												VALUES(
													'" . Date('Y-m-d H:i:s') . "',
													'" . trim($_SESSION['UserID']) . "',
													'" . $_SERVER['REMOTE_ADDR'] . "',
													'" . DB_escape_string($SQL) . "')";

				$AuditResult = mysqli_query($db, $AuditSQL);
			}
		}
	}
	return $Result;

}

function DB_fetch_row(&$ResultIndex) {

	$RowPointer = mysqli_fetch_row($ResultIndex);
	return $RowPointer;
}

function DB_fetch_assoc(&$ResultIndex) {

	$RowPointer = mysqli_fetch_assoc($ResultIndex);
	return $RowPointer;
}

function DB_fetch_array(&$ResultIndex) {

	$RowPointer = mysqli_fetch_array($ResultIndex);
	return $RowPointer;
}

function DB_fetch_all(&$ResultIndex) {

	$ResultArray = mysqli_fetch_all($ResultIndex, MYSQLI_ASSOC);
	return $ResultArray;
}

function DB_data_seek(&$ResultIndex, $Record) {
	mysqli_data_seek($ResultIndex, $Record);
}

function DB_free_result(&$ResultIndex) {
	if (is_resource($ResultIndex)) {
		mysqli_free_result($ResultIndex);
	}
}

function DB_num_rows(&$ResultIndex) {
	return mysqli_num_rows($ResultIndex);
}

function DB_affected_rows(&$ResultIndex) {

	global $db;
	return mysqli_affected_rows($db);

}

function DB_error_no() {
	global $db;
	return mysqli_errno($db);
}

function DB_error_msg() {
	global $db;
	return mysqli_error($db);
}

function DB_Last_Insert_ID($Table, $FieldName) {
	if (isset($_SESSION['LastInsertId'])) {
		$Last_Insert_ID = $_SESSION['LastInsertId'];
	} else {
		$Last_Insert_ID = 0;
	}
	//	unset($_SESSION['LastInsertId']);
	return $Last_Insert_ID;
}

function DB_escape_string($String) {
	global $db;
	return mysqli_real_escape_string($db, $String);
}

function DB_show_tables($TableName = '%') {
	$Result = DB_query("SHOW TABLES WHERE Tables_in_" . $_SESSION['DatabaseName'] . " " . LIKE . "'" . $TableName . "'");
	return $Result;
}

function DB_show_fields($TableName) {
	$Result = DB_query("DESCRIBE $TableName");
	return $Result;
}

function interval($Value, $Interval) {
	global $DBType;
	return "\n" . 'interval ' . $Value . ' ' . $Interval . "\n";
}

function DB_Maintenance() {

	prnMsg(_('The system has just run the regular database administration and optimisation routine.'), 'info');

	$TablesResult = DB_query('SHOW TABLES');
	while ($MyRow = DB_fetch_row($TablesResult)) {
		$Result = DB_query('OPTIMIZE TABLE ' . $MyRow[0]);
	}

	$Result = DB_query("UPDATE config
				SET confvalue=CURRENT_DATE
				WHERE confname='DB_Maintenance_LastRun'");
}

function DB_Txn_Begin() {
	global $db;
	mysqli_query($db, 'SET autocommit=0');
	mysqli_query($db, 'START TRANSACTION');
}

function DB_Txn_Commit() {
	global $db;
	mysqli_query($db, 'COMMIT');
	mysqli_query($db, 'SET autocommit=1');
}

function DB_Txn_Rollback() {
	global $db;
	mysqli_query($db, 'ROLLBACK');
}
function DB_IgnoreForeignKeys() {
	global $db;
	mysqli_query($db, 'SET FOREIGN_KEY_CHECKS=0');
}
function DB_ReinstateForeignKeys() {
	global $db;
	mysqli_query($db, 'SET FOREIGN_KEY_CHECKS=1');
}

function DB_table_exists($TableName) {
	$ShowSQL = "SHOW TABLES WHERE Tables_in_" . $_SESSION['DatabaseName'] . "='" . $TableName . "'";
	$Result = DB_query($ShowSQL);
	if (DB_num_rows($Result) > 0) {
		return True;
	} else {
		return False;
	}
}

function DB_select_database($DBName) {
	global $db;
	mysqli_select_db($db, $DBName);
}

function DB_set_timezone() {
	$Now = new DateTime();
	$Minutes = $Now->getOffset() / 60;
	$Sign = ($Minutes < 0 ? -1 : 1);
	$Minutes = abs($Minutes);
	$Hours = floor($Minutes / 60);
	$Minutes -= $Hours * 60;
	$Offset = sprintf('%+d:%02d', $Hours * $Sign, $Minutes);
	$Result = DB_query("SET time_zone='" . $Offset . "'");
}

?>