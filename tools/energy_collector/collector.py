# Copyright (c) 2020-2021 Adrian Georg Herrmann

import sys
import math
import hashlib
import json
import xml.etree.ElementTree as ET
from datetime import datetime, date
import time
import http.client
import smtplib
from email.message import EmailMessage


log = ""

def print_and_log(string, exit=False):
    global log
    msg = "[{}] {}\n".format(str(datetime.fromtimestamp(time.time())), string)
    log += msg
    with open("log.txt", "a") as log_file:
        log_file.write(msg)

    if exit:
        sys.exit(string)
    else:
        print(string)


class Collector:

    cfg = None
    conn = None
    default_sid = "0000000000000000"
    sid = default_sid

    def _get_element_from_login_xml(self, xml, element):
        xml_root = ET.fromstring(xml)
        if xml_root.tag != "SessionInfo":
            print_and_log("Invalid response from FRITZ!Box, could not login: {}".format(xml), exit=True)

        element_ = xml_root.find(element)
        if element_ is None:
            print_and_log("Could not find '{}' in FRITZ!Box response, could not login: {}".format(element, xml), exit=True)
        else:
            return element_.text

    def _get_http_response(self, resource):
        self.conn.request("GET", resource)
        conn_response = self.conn.getresponse()
        if conn_response.status != 200:
            print_and_log(
                "Could not connect to FRITZ!Box at {}, terminating. (HTTP response {})".format(self.cfg["fritz_ip"], conn_response.status),
                exit=True
            )
        else:
            return conn_response

    # def _send_command(self, ain, switchcmd, params=None):
    #     cmd = "/webservices/homeautoswitch.lua?{}switchcmd={}&sid={}".format("" if ain is None else "ain={}&".format(ain), switchcmd, self.sid)
    #     if type(params) is dict:
    #         for key, value in params:
    #             cmd += "&{}={}".format(key, value)
    #     return self._get_http_response(cmd).read().decode("utf-8")

    def _send_home_auto_query(self, id, command):
        cmd = "/net/home_auto_query.lua?sid={}&command={}&id={}&xhr=1".format(self.sid, command, id)
        return self._get_http_response(cmd).read().decode("utf-8")

    def get_energy_24h(self):
        num_tries = 0
        while True:
            response = json.loads(self._send_home_auto_query(self.cfg["switch_id"], "EnergyStats_24h"))
            if response.get("EnergyStat") is None or response["EnergyStat"].get("values") is None:
                if num_tries < 2:
                    num_tries += 1
                    time.sleep(5)
                    continue
                else:
                    print_and_log("No energy stat in FRITZ!Box response, terminating: {}".format(response), exit=True)
            elif response.get("CurrentDateInSec") is None:
                print_and_log("No current timestamp in FRITZ!Box response, terminating: {}".format(response), exit=True)
            else:
                energy_list = response["EnergyStat"]["values"]
                return int(response["CurrentDateInSec"]), energy_list

    def send_email(self, source, target, subject, content=None, attachment=None, log=True):
        msg = EmailMessage()

        msg["From"] = source
        msg["To"] = target
        msg["Subject"] = subject

        if content is not None:
            msg.set_content(content)

        if (type(attachment) is dict and
            attachment.get("content") and attachment.get("maintype") and
            attachment.get("subtype") and attachment.get("filename")
        ):
            msg.add_attachment(
                attachment["content"],
                maintype=attachment["maintype"],
                subtype=attachment["subtype"],
                filename=attachment["filename"]
            )

        if log:
            print_and_log(
                "Sending email from '{}' to '{}' with subject '{}': {} (attachment: {})"
                    .format(source, target, subject, content, "none" if attachment is None else attachment)
            )

        with smtplib.SMTP(self.cfg["email_server"]) as server:
            server.connect(self.cfg["email_server"], self.cfg["email_port"])
            server.ehlo()
            server.starttls()
            server.ehlo()
            server.login(self.cfg["email_from"], self.cfg["email_pass"])
            server.send_message(msg)
            server.quit()

        if log:
            print_and_log("Email sent successfully.")

    def collect_and_send(self):
        timestamp, energy_list = self.get_energy_24h()

        last_datetime_ = datetime.fromtimestamp(timestamp)
        last_datetime = last_datetime_.replace(minute=math.floor(last_datetime_.minute / 15) * 15, second=0, microsecond=0)
        last_timestamp = last_datetime.timestamp()

        file_name = "{}.csv".format(str(last_datetime))
        file_content = ""

        for i, energy_item in enumerate(energy_list):
            cur_datetime = datetime.fromtimestamp(last_timestamp - i*900)
            file_content += "{};{}\n".format(str(cur_datetime), energy_item)

        subject = "[HEMS] [data] [{}] {}".format(self.cfg["id"], last_datetime)
        attachment = {
            "content": file_content.encode('utf-8'),
            "maintype": "text",
            "subtype": "csv",
            "filename": file_name
        }
        self.send_email(self.cfg["email_from"], self.cfg["email_to"], subject, attachment=attachment)

    def fritz_logout(self):
        print_and_log("Logging out of FRITZ!Box session with SID: {}.".format(self.sid))
        lua = "/login_sid.lua"
        self._get_http_response("{}?logout=&sid={}".format(lua, self.sid))
        print_and_log("FRITZ!Box logout finished (SID: {}).".format(self.sid))
        self.sid = self.default_sid

    def fritz_login(self):
        print_and_log("Logging in to FRITZ!Box.")

        lua = "/login_sid.lua"

        xml = self._get_http_response(lua).read()
        challenge = self._get_element_from_login_xml(xml, "Challenge")

        pwd = list(self.cfg["fritz_pass"])
        for i, char in enumerate(pwd):
            if ord(char) > 255:
                pwd[i] = '.'
        pwd = "{}-{}".format(challenge, "".join(pwd)).encode('utf-16le')

        response = "{}-{}".format(challenge, hashlib.md5(pwd).hexdigest())

        xml = self._get_http_response("{}?username={}&response={}".format(lua, self.cfg["fritz_user"], response)).read()
        sid = self._get_element_from_login_xml(xml, "SID")
        if sid == self.default_sid:
            print_and_log(
                "Failed to get valid SID from FRITZ!Box, could not login with response '{}': {}".format(response, xml),
                exit=True
            )
        else:
            self.sid = sid
            print_and_log("FRITZ!Box login successful with SID: {}".format(self.sid))

    def _check_config(self, cfg):
        if cfg.get("id") is None:
            return "identity"
        elif cfg.get("email_from") is None:
            return "sender email address"
        elif cfg.get("email_to") is None:
            return "destination email address"
        elif cfg.get("email_pass") is None:
            return "email password"
        elif cfg.get("email_server") is None:
            return "email server"
        elif cfg.get("email_port") is None:
            return "email port"
        elif cfg.get("fritz_ip") is None:
            return "FRITZ!Box (IP) address"
        elif cfg.get("fritz_user") is None:
            return "FRITZ!Box username"
        elif cfg.get("fritz_pass") is None:
            return "FRITZ!Box password"
        elif cfg.get("switch_id") is None:
            return "switch ID"
        return None

    def load_config(self, cfg_path="config.json"):
        with open(cfg_path, "r") as cfg_file:
            self.cfg = json.load(cfg_file)
            error = self._check_config(self.cfg)
            cfg_print = str(self.cfg).replace(self.cfg["fritz_pass"], "***").replace(self.cfg["email_pass"], "***")
            if error:
                print_and_log("No {} provided, terminating. (Configuration: {})".format(error, cfg_print), exit=True)
            else:
                print_and_log("Start with valid configuration: {}".format(cfg_print))

    def __init__(self):
        self.load_config()
        self.conn = http.client.HTTPConnection(self.cfg["fritz_ip"])


if __name__ == "__main__":
    collector = Collector()
    try:
        collector.fritz_login()
        collector.collect_and_send()
        print_and_log("Success, terminating.")
    except Exception as e:
        log += str(e)
        raise e
    finally:
        collector.fritz_logout()
        collector.send_email(
            collector.cfg["email_from"],
            collector.cfg["email_to"],
            "[HEMS] [log] [{}] {}".format(collector.cfg["id"], str(datetime.fromtimestamp(time.time()))),
            content=log,
            log=False
        )
