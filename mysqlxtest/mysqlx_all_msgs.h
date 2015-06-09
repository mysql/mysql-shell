/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

template<class C>
mysqlx::Message *create()
{
  return new C();
}

static struct init_message_factory
{
  init_message_factory()
  {
    server_msgs_by_name["SQL_CURSOR_FETCH_SUSPENDED"] = std::make_pair(&create<Mysqlx::Sql::CursorFetchSuspended>, Mysqlx::ServerMessages::SQL_CURSOR_FETCH_SUSPENDED);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_CURSOR_FETCH_SUSPENDED] = std::make_pair(&create<Mysqlx::Sql::CursorFetchSuspended>, "SQL_CURSOR_FETCH_SUSPENDED");
    server_msgs_by_full_name["Mysqlx.Sql.CursorFetchSuspended"] = "SQL_CURSOR_FETCH_SUSPENDED";
    server_msgs_by_name["SQL_CURSORS_POLL"] = std::make_pair(&create<Mysqlx::Sql::CursorsPoll>, Mysqlx::ServerMessages::SQL_CURSORS_POLL);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_CURSORS_POLL] = std::make_pair(&create<Mysqlx::Sql::CursorsPoll>, "SQL_CURSORS_POLL");
    server_msgs_by_full_name["Mysqlx.Sql.CursorsPoll"] = "SQL_CURSORS_POLL";
    server_msgs_by_name["NOTICE"] = std::make_pair(&create<Mysqlx::Notice>, Mysqlx::ServerMessages::NOTICE);
    server_msgs_by_id[Mysqlx::ServerMessages::NOTICE] = std::make_pair(&create<Mysqlx::Notice>, "NOTICE");
    server_msgs_by_full_name["Mysqlx.Notice"] = "NOTICE";
    server_msgs_by_name["SESS_AUTHENTICATE_CONTINUE"] = std::make_pair(&create<Mysqlx::Session::AuthenticateContinue>, Mysqlx::ServerMessages::SESS_AUTHENTICATE_CONTINUE);
    server_msgs_by_id[Mysqlx::ServerMessages::SESS_AUTHENTICATE_CONTINUE] = std::make_pair(&create<Mysqlx::Session::AuthenticateContinue>, "SESS_AUTHENTICATE_CONTINUE");
    server_msgs_by_full_name["Mysqlx.Session.AuthenticateContinue"] = "SESS_AUTHENTICATE_CONTINUE";
    server_msgs_by_name["SQL_PREPARE_STMT_OK"] = std::make_pair(&create<Mysqlx::Sql::PrepareStmtOk>, Mysqlx::ServerMessages::SQL_PREPARE_STMT_OK);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_PREPARE_STMT_OK] = std::make_pair(&create<Mysqlx::Sql::PrepareStmtOk>, "SQL_PREPARE_STMT_OK");
    server_msgs_by_full_name["Mysqlx.Sql.PrepareStmtOk"] = "SQL_PREPARE_STMT_OK";
    server_msgs_by_name["SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS"] = std::make_pair(&create<Mysqlx::Sql::CursorFetchDoneMoreResultsets>, Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS] = std::make_pair(&create<Mysqlx::Sql::CursorFetchDoneMoreResultsets>, "SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS");
    server_msgs_by_full_name["Mysqlx.Sql.CursorFetchDoneMoreResultsets"] = "SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS";
    server_msgs_by_name["CONN_CAPABILITIES"] = std::make_pair(&create<Mysqlx::Connection::Capabilities>, Mysqlx::ServerMessages::CONN_CAPABILITIES);
    server_msgs_by_id[Mysqlx::ServerMessages::CONN_CAPABILITIES] = std::make_pair(&create<Mysqlx::Connection::Capabilities>, "CONN_CAPABILITIES");
    server_msgs_by_full_name["Mysqlx.Connection.Capabilities"] = "CONN_CAPABILITIES";
    server_msgs_by_name["SQL_ROW"] = std::make_pair(&create<Mysqlx::Sql::Row>, Mysqlx::ServerMessages::SQL_ROW);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_ROW] = std::make_pair(&create<Mysqlx::Sql::Row>, "SQL_ROW");
    server_msgs_by_full_name["Mysqlx.Sql.Row"] = "SQL_ROW";
    server_msgs_by_name["SQL_CURSOR_FETCH_DONE"] = std::make_pair(&create<Mysqlx::Sql::CursorFetchDone>, Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE] = std::make_pair(&create<Mysqlx::Sql::CursorFetchDone>, "SQL_CURSOR_FETCH_DONE");
    server_msgs_by_full_name["Mysqlx.Sql.CursorFetchDone"] = "SQL_CURSOR_FETCH_DONE";
    server_msgs_by_name["SQL_STMT_EXECUTE_OK"] = std::make_pair(&create<Mysqlx::Sql::StmtExecuteOk>, Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK] = std::make_pair(&create<Mysqlx::Sql::StmtExecuteOk>, "SQL_STMT_EXECUTE_OK");
    server_msgs_by_full_name["Mysqlx.Sql.StmtExecuteOk"] = "SQL_STMT_EXECUTE_OK";
    server_msgs_by_name["PARAMETER_CHANGED_NOTIFICATION"] = std::make_pair(&create<Mysqlx::ParameterChangedNotification>, Mysqlx::ServerMessages::PARAMETER_CHANGED_NOTIFICATION);
    server_msgs_by_id[Mysqlx::ServerMessages::PARAMETER_CHANGED_NOTIFICATION] = std::make_pair(&create<Mysqlx::ParameterChangedNotification>, "PARAMETER_CHANGED_NOTIFICATION");
    server_msgs_by_full_name["Mysqlx.ParameterChangedNotification"] = "PARAMETER_CHANGED_NOTIFICATION";
    server_msgs_by_name["SESS_AUTHENTICATE_FAIL"] = std::make_pair(&create<Mysqlx::Session::AuthenticateFail>, Mysqlx::ServerMessages::SESS_AUTHENTICATE_FAIL);
    server_msgs_by_id[Mysqlx::ServerMessages::SESS_AUTHENTICATE_FAIL] = std::make_pair(&create<Mysqlx::Session::AuthenticateFail>, "SESS_AUTHENTICATE_FAIL");
    server_msgs_by_full_name["Mysqlx.Session.AuthenticateFail"] = "SESS_AUTHENTICATE_FAIL";
    server_msgs_by_name["SQL_CURSOR_CLOSE_OK"] = std::make_pair(&create<Mysqlx::Sql::CursorCloseOk>, Mysqlx::ServerMessages::SQL_CURSOR_CLOSE_OK);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_CURSOR_CLOSE_OK] = std::make_pair(&create<Mysqlx::Sql::CursorCloseOk>, "SQL_CURSOR_CLOSE_OK");
    server_msgs_by_full_name["Mysqlx.Sql.CursorCloseOk"] = "SQL_CURSOR_CLOSE_OK";
    server_msgs_by_name["ERROR"] = std::make_pair(&create<Mysqlx::Error>, Mysqlx::ServerMessages::ERROR);
    server_msgs_by_id[Mysqlx::ServerMessages::ERROR] = std::make_pair(&create<Mysqlx::Error>, "ERROR");
    server_msgs_by_full_name["Mysqlx.Error"] = "ERROR";
    server_msgs_by_name["OK"] = std::make_pair(&create<Mysqlx::Ok>, Mysqlx::ServerMessages::OK);
    server_msgs_by_id[Mysqlx::ServerMessages::OK] = std::make_pair(&create<Mysqlx::Ok>, "OK");
    server_msgs_by_full_name["Mysqlx.Ok"] = "OK";
    server_msgs_by_name["SQL_COLUMN_META_DATA"] = std::make_pair(&create<Mysqlx::Sql::ColumnMetaData>, Mysqlx::ServerMessages::SQL_COLUMN_META_DATA);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_COLUMN_META_DATA] = std::make_pair(&create<Mysqlx::Sql::ColumnMetaData>, "SQL_COLUMN_META_DATA");
    server_msgs_by_full_name["Mysqlx.Sql.ColumnMetaData"] = "SQL_COLUMN_META_DATA";
    server_msgs_by_name["SESS_AUTHENTICATE_OK"] = std::make_pair(&create<Mysqlx::Session::AuthenticateOk>, Mysqlx::ServerMessages::SESS_AUTHENTICATE_OK);
    server_msgs_by_id[Mysqlx::ServerMessages::SESS_AUTHENTICATE_OK] = std::make_pair(&create<Mysqlx::Session::AuthenticateOk>, "SESS_AUTHENTICATE_OK");
    server_msgs_by_full_name["Mysqlx.Session.AuthenticateOk"] = "SESS_AUTHENTICATE_OK";
    server_msgs_by_name["SQL_PREPARED_STMT_EXECUTE_OK"] = std::make_pair(&create<Mysqlx::Sql::PreparedStmtExecuteOk>, Mysqlx::ServerMessages::SQL_PREPARED_STMT_EXECUTE_OK);
    server_msgs_by_id[Mysqlx::ServerMessages::SQL_PREPARED_STMT_EXECUTE_OK] = std::make_pair(&create<Mysqlx::Sql::PreparedStmtExecuteOk>, "SQL_PREPARED_STMT_EXECUTE_OK");
    server_msgs_by_full_name["Mysqlx.Sql.PreparedStmtExecuteOk"] = "SQL_PREPARED_STMT_EXECUTE_OK";

    client_msgs_by_name["SQL_CURSOR_FETCH_META_DATA"] = std::make_pair(&create<Mysqlx::Sql::CursorFetchMetaData>, Mysqlx::ClientMessages::SQL_CURSOR_FETCH_META_DATA);
    client_msgs_by_id[Mysqlx::ClientMessages::SQL_CURSOR_FETCH_META_DATA] = std::make_pair(&create<Mysqlx::Sql::CursorFetchMetaData>, "SQL_CURSOR_FETCH_META_DATA");
    client_msgs_by_full_name["Mysqlx.Sql.CursorFetchMetaData"] = "SQL_CURSOR_FETCH_META_DATA";
    client_msgs_by_name["CON_CAPABILITIES_GET"] = std::make_pair(&create<Mysqlx::Connection::CapabilitiesGet>, Mysqlx::ClientMessages::CON_CAPABILITIES_GET);
    client_msgs_by_id[Mysqlx::ClientMessages::CON_CAPABILITIES_GET] = std::make_pair(&create<Mysqlx::Connection::CapabilitiesGet>, "CON_CAPABILITIES_GET");
    client_msgs_by_full_name["Mysqlx.Connection.CapabilitiesGet"] = "CON_CAPABILITIES_GET";
    client_msgs_by_name["CRUD_UPDATE"] = std::make_pair(&create<Mysqlx::Crud::Update>, Mysqlx::ClientMessages::CRUD_UPDATE);
    client_msgs_by_id[Mysqlx::ClientMessages::CRUD_UPDATE] = std::make_pair(&create<Mysqlx::Crud::Update>, "CRUD_UPDATE");
    client_msgs_by_full_name["Mysqlx.Crud.Update"] = "CRUD_UPDATE";
    client_msgs_by_name["SQL_PREPARED_STMT_EXECUTE"] = std::make_pair(&create<Mysqlx::Sql::PreparedStmtExecute>, Mysqlx::ClientMessages::SQL_PREPARED_STMT_EXECUTE);
    client_msgs_by_id[Mysqlx::ClientMessages::SQL_PREPARED_STMT_EXECUTE] = std::make_pair(&create<Mysqlx::Sql::PreparedStmtExecute>, "SQL_PREPARED_STMT_EXECUTE");
    client_msgs_by_full_name["Mysqlx.Sql.PreparedStmtExecute"] = "SQL_PREPARED_STMT_EXECUTE";
    client_msgs_by_name["SESS_AUTHENTICATE_CONTINUE"] = std::make_pair(&create<Mysqlx::Session::AuthenticateContinue>, Mysqlx::ClientMessages::SESS_AUTHENTICATE_CONTINUE);
    client_msgs_by_id[Mysqlx::ClientMessages::SESS_AUTHENTICATE_CONTINUE] = std::make_pair(&create<Mysqlx::Session::AuthenticateContinue>, "SESS_AUTHENTICATE_CONTINUE");
    client_msgs_by_full_name["Mysqlx.Session.AuthenticateContinue"] = "SESS_AUTHENTICATE_CONTINUE";
    client_msgs_by_name["SQL_PREPARED_STMT_CLOSE"] = std::make_pair(&create<Mysqlx::Sql::PreparedStmtClose>, Mysqlx::ClientMessages::SQL_PREPARED_STMT_CLOSE);
    client_msgs_by_id[Mysqlx::ClientMessages::SQL_PREPARED_STMT_CLOSE] = std::make_pair(&create<Mysqlx::Sql::PreparedStmtClose>, "SQL_PREPARED_STMT_CLOSE");
    client_msgs_by_full_name["Mysqlx.Sql.PreparedStmtClose"] = "SQL_PREPARED_STMT_CLOSE";
    client_msgs_by_name["CON_CAPABILITIES_SET"] = std::make_pair(&create<Mysqlx::Connection::CapabilitiesSet>, Mysqlx::ClientMessages::CON_CAPABILITIES_SET);
    client_msgs_by_id[Mysqlx::ClientMessages::CON_CAPABILITIES_SET] = std::make_pair(&create<Mysqlx::Connection::CapabilitiesSet>, "CON_CAPABILITIES_SET");
    client_msgs_by_full_name["Mysqlx.Connection.CapabilitiesSet"] = "CON_CAPABILITIES_SET";
    client_msgs_by_name["CRUD_DELETE"] = std::make_pair(&create<Mysqlx::Crud::Delete>, Mysqlx::ClientMessages::CRUD_DELETE);
    client_msgs_by_id[Mysqlx::ClientMessages::CRUD_DELETE] = std::make_pair(&create<Mysqlx::Crud::Delete>, "CRUD_DELETE");
    client_msgs_by_full_name["Mysqlx.Crud.Delete"] = "CRUD_DELETE";
    client_msgs_by_name["SQL_PREPARE_STMT"] = std::make_pair(&create<Mysqlx::Sql::PrepareStmt>, Mysqlx::ClientMessages::SQL_PREPARE_STMT);
    client_msgs_by_id[Mysqlx::ClientMessages::SQL_PREPARE_STMT] = std::make_pair(&create<Mysqlx::Sql::PrepareStmt>, "SQL_PREPARE_STMT");
    client_msgs_by_full_name["Mysqlx.Sql.PrepareStmt"] = "SQL_PREPARE_STMT";
    client_msgs_by_name["SQL_CURSOR_FETCH_ROWS"] = std::make_pair(&create<Mysqlx::Sql::CursorFetchRows>, Mysqlx::ClientMessages::SQL_CURSOR_FETCH_ROWS);
    client_msgs_by_id[Mysqlx::ClientMessages::SQL_CURSOR_FETCH_ROWS] = std::make_pair(&create<Mysqlx::Sql::CursorFetchRows>, "SQL_CURSOR_FETCH_ROWS");
    client_msgs_by_full_name["Mysqlx.Sql.CursorFetchRows"] = "SQL_CURSOR_FETCH_ROWS";
    client_msgs_by_name["CRUD_INSERT"] = std::make_pair(&create<Mysqlx::Crud::Insert>, Mysqlx::ClientMessages::CRUD_INSERT);
    client_msgs_by_id[Mysqlx::ClientMessages::CRUD_INSERT] = std::make_pair(&create<Mysqlx::Crud::Insert>, "CRUD_INSERT");
    client_msgs_by_full_name["Mysqlx.Crud.Insert"] = "CRUD_INSERT";
    client_msgs_by_name["SESS_CLOSE"] = std::make_pair(&create<Mysqlx::Session::Close>, Mysqlx::ClientMessages::SESS_CLOSE);
    client_msgs_by_id[Mysqlx::ClientMessages::SESS_CLOSE] = std::make_pair(&create<Mysqlx::Session::Close>, "SESS_CLOSE");
    client_msgs_by_full_name["Mysqlx.Session.Close"] = "SESS_CLOSE";
    client_msgs_by_name["ADMIN_COMMAND_EXECUTE"] = std::make_pair(&create<Mysqlx::Admin::CommandExecute>, Mysqlx::ClientMessages::ADMIN_COMMAND_EXECUTE);
    client_msgs_by_id[Mysqlx::ClientMessages::ADMIN_COMMAND_EXECUTE] = std::make_pair(&create<Mysqlx::Admin::CommandExecute>, "ADMIN_COMMAND_EXECUTE");
    client_msgs_by_full_name["Mysqlx.Admin.CommandExecute"] = "ADMIN_COMMAND_EXECUTE";
    client_msgs_by_name["SQL_CURSOR_CLOSE"] = std::make_pair(&create<Mysqlx::Sql::CursorClose>, Mysqlx::ClientMessages::SQL_CURSOR_CLOSE);
    client_msgs_by_id[Mysqlx::ClientMessages::SQL_CURSOR_CLOSE] = std::make_pair(&create<Mysqlx::Sql::CursorClose>, "SQL_CURSOR_CLOSE");
    client_msgs_by_full_name["Mysqlx.Sql.CursorClose"] = "SQL_CURSOR_CLOSE";
    client_msgs_by_name["CRUD_PREPARE_UPDATE"] = std::make_pair(&create<Mysqlx::Crud::PrepareUpdate>, Mysqlx::ClientMessages::CRUD_PREPARE_UPDATE);
    client_msgs_by_id[Mysqlx::ClientMessages::CRUD_PREPARE_UPDATE] = std::make_pair(&create<Mysqlx::Crud::PrepareUpdate>, "CRUD_PREPARE_UPDATE");
    client_msgs_by_full_name["Mysqlx.Crud.PrepareUpdate"] = "CRUD_PREPARE_UPDATE";
    client_msgs_by_name["SQL_STMT_EXECUTE"] = std::make_pair(&create<Mysqlx::Sql::StmtExecute>, Mysqlx::ClientMessages::SQL_STMT_EXECUTE);
    client_msgs_by_id[Mysqlx::ClientMessages::SQL_STMT_EXECUTE] = std::make_pair(&create<Mysqlx::Sql::StmtExecute>, "SQL_STMT_EXECUTE");
    client_msgs_by_full_name["Mysqlx.Sql.StmtExecute"] = "SQL_STMT_EXECUTE";
    client_msgs_by_name["CRUD_PREPARE_INSERT"] = std::make_pair(&create<Mysqlx::Crud::PrepareInsert>, Mysqlx::ClientMessages::CRUD_PREPARE_INSERT);
    client_msgs_by_id[Mysqlx::ClientMessages::CRUD_PREPARE_INSERT] = std::make_pair(&create<Mysqlx::Crud::PrepareInsert>, "CRUD_PREPARE_INSERT");
    client_msgs_by_full_name["Mysqlx.Crud.PrepareInsert"] = "CRUD_PREPARE_INSERT";
    client_msgs_by_name["CRUD_PREPARE_FIND"] = std::make_pair(&create<Mysqlx::Crud::PrepareFind>, Mysqlx::ClientMessages::CRUD_PREPARE_FIND);
    client_msgs_by_id[Mysqlx::ClientMessages::CRUD_PREPARE_FIND] = std::make_pair(&create<Mysqlx::Crud::PrepareFind>, "CRUD_PREPARE_FIND");
    client_msgs_by_full_name["Mysqlx.Crud.PrepareFind"] = "CRUD_PREPARE_FIND";
    client_msgs_by_name["SESS_RESET"] = std::make_pair(&create<Mysqlx::Session::Reset>, Mysqlx::ClientMessages::SESS_RESET);
    client_msgs_by_id[Mysqlx::ClientMessages::SESS_RESET] = std::make_pair(&create<Mysqlx::Session::Reset>, "SESS_RESET");
    client_msgs_by_full_name["Mysqlx.Session.Reset"] = "SESS_RESET";
    client_msgs_by_name["CON_CLOSE"] = std::make_pair(&create<Mysqlx::Connection::Close>, Mysqlx::ClientMessages::CON_CLOSE);
    client_msgs_by_id[Mysqlx::ClientMessages::CON_CLOSE] = std::make_pair(&create<Mysqlx::Connection::Close>, "CON_CLOSE");
    client_msgs_by_full_name["Mysqlx.Connection.Close"] = "CON_CLOSE";
    client_msgs_by_name["CRUD_FIND"] = std::make_pair(&create<Mysqlx::Crud::Find>, Mysqlx::ClientMessages::CRUD_FIND);
    client_msgs_by_id[Mysqlx::ClientMessages::CRUD_FIND] = std::make_pair(&create<Mysqlx::Crud::Find>, "CRUD_FIND");
    client_msgs_by_full_name["Mysqlx.Crud.Find"] = "CRUD_FIND";
    client_msgs_by_name["CRUD_PREPARE_DELETE"] = std::make_pair(&create<Mysqlx::Crud::PrepareDelete>, Mysqlx::ClientMessages::CRUD_PREPARE_DELETE);
    client_msgs_by_id[Mysqlx::ClientMessages::CRUD_PREPARE_DELETE] = std::make_pair(&create<Mysqlx::Crud::PrepareDelete>, "CRUD_PREPARE_DELETE");
    client_msgs_by_full_name["Mysqlx.Crud.PrepareDelete"] = "CRUD_PREPARE_DELETE";
    client_msgs_by_name["SQL_CURSORS_POLL"] = std::make_pair(&create<Mysqlx::Sql::CursorsPoll>, Mysqlx::ClientMessages::SQL_CURSORS_POLL);
    client_msgs_by_id[Mysqlx::ClientMessages::SQL_CURSORS_POLL] = std::make_pair(&create<Mysqlx::Sql::CursorsPoll>, "SQL_CURSORS_POLL");
    client_msgs_by_full_name["Mysqlx.Sql.CursorsPoll"] = "SQL_CURSORS_POLL";
    client_msgs_by_name["SESS_AUTHENTICATE_START"] = std::make_pair(&create<Mysqlx::Session::AuthenticateStart>, Mysqlx::ClientMessages::SESS_AUTHENTICATE_START);
    client_msgs_by_id[Mysqlx::ClientMessages::SESS_AUTHENTICATE_START] = std::make_pair(&create<Mysqlx::Session::AuthenticateStart>, "SESS_AUTHENTICATE_START");
    client_msgs_by_full_name["Mysqlx.Session.AuthenticateStart"] = "SESS_AUTHENTICATE_START";
  }
} init_message_factory;


