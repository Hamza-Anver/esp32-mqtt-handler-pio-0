#include "webpage.h"
#include <Arduino.h>
const char FULL_CONFIG_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1">
    <title> SLT DL Configuration</title>
    <style>
        :root,
        ::backdrop {
            --sans-font: -apple-system, BlinkMacSystemFont, "Avenir Next", Avenir, "Nimbus Sans L", Roboto, "Noto Sans", "Segoe UI", Arial, Helvetica, "Helvetica Neue", sans-serif;
            --mono-font: Consolas, Menlo, Monaco, "Andale Mono", "Ubuntu Mono", monospace;
            --standard-border-radius: 5px;
            --bg: #fff;
            --accent-bg: #f5f7ff;
            --text: #212121;
            --text-light: #585858;
            --border: #898ea4;
            --accent: #0d47a1;
            --accent-hover: #1266e2;
            --accent-text: var(--bg);
            --code: #d81b60;
            --preformatted: #444;
            --marked: #fd3;
            --disabled: #efefef
        }

        @media (prefers-color-scheme:dark) {

            :root,
            ::backdrop {
                color-scheme: dark;
                --bg: #212121;
                --accent-bg: #2b2b2b;
                --text: #dcdcdc;
                --text-light: #ababab;
                --accent: #ffb300;
                --accent-hover: #ffe099;
                --accent-text: var(--bg);
                --code: #f06292;
                --preformatted: #ccc;
                --disabled: #111;
            }

            img,
            video {
                opacity: .8
            }
        }

        *,
        :before,
        :after {
            box-sizing: border-box
        }

        textarea,
        select,
        input,
        progress {
            -webkit-appearance: none;
            -moz-appearance: none;
            appearance: none
        }

        html {
            font-family: var(--sans-font);
            scroll-behavior: smooth
        }

        body {
            color: var(--text);
            background-color: var(--bg);
            grid-template-columns: 1fr min(45rem, 90%) 1fr;
            margin: 0;
            font-size: 1.15rem;
            line-height: 1.5;
            display: grid
        }

        body>* {
            grid-column: 2
        }

        body>header {
            background-color: var(--accent-bg);
            border-bottom: 1px solid var(--border);
            text-align: center;
            grid-column: 1/-1;
            padding: 0 .5rem 2rem
        }

        body>header>:only-child {
            margin-block-start: 2rem
        }

        body>header h1 {
            max-width: 1200px;
            margin: 1rem auto
        }

        body>header p {
            max-width: 40rem;
            margin: 1rem auto
        }

        main {
            padding-top: 1.5rem
        }

        footer {
            color: var(--text-light);
            text-align: center;
            padding: 0.8rem;
            margin: 0.5rem;
            font-size: .9rem
        }

        h1 {
            font-size: 3rem
        }

        h2 {
            margin-top: 3rem;
            font-size: 2.6rem
        }

        h3 {
            margin-top: 3rem;
            font-size: 2rem
        }

        h4 {
            font-size: 1.44rem
        }

        h5 {
            font-size: 1.15rem
        }

        h6 {
            font-size: .96rem
        }

        p {
            margin: 1.5rem 0
        }

        p,
        h1,
        h2,
        h3,
        h4,
        h5,
        h6 {
            overflow-wrap: break-word
        }

        h1,
        h2,
        h3 {
            line-height: 1.1
        }

        @media only screen and (width<=720px) {
            h1 {
                font-size: 2.5rem
            }

            h2 {
                font-size: 2.1rem
            }

            h3 {
                font-size: 1.75rem
            }

            h4 {
                font-size: 1.25rem
            }
        }

        a,
        a:visited {
            color: var(--accent)
        }

        a:hover {
            text-decoration: none
        }

        button,
        .button,
        a.button,
        input[type=submit],
        input[type=reset],
        input[type=button],
        label[type=button] {
            border: 1px solid var(--accent);
            background-color: var(--accent);
            color: var(--accent-text);
            padding: .5rem .9rem;
            line-height: normal;
            text-decoration: none
        }

        .button[aria-disabled=true],
        input:disabled,
        textarea:disabled,
        select:disabled,
        button[disabled] {
            cursor: not-allowed;
            background-color: var(--disabled);
            border-color: var(--disabled);
            color: var(--text-light)
        }

        input[type=range] {
            padding: 0
        }

        abbr[title] {
            cursor: help;
            text-decoration-line: underline;
            text-decoration-style: dotted
        }

        button:enabled:hover,
        .button:not([aria-disabled=true]):hover,
        input[type=submit]:enabled:hover,
        input[type=reset]:enabled:hover,
        input[type=button]:enabled:hover,
        label[type=button]:hover {
            background-color: var(--accent-hover);
            border-color: var(--accent-hover);
            cursor: pointer
        }

        .button:focus-visible,
        button:focus-visible:where(:enabled),
        input:enabled:focus-visible:where([type=submit], [type=reset], [type=button]) {
            outline: 2px solid var(--accent);
            outline-offset: 1px
        }

        header>nav {
            padding: 1rem 0 0;
            font-size: 1rem;
            line-height: 2
        }

        header>nav ul,
        header>nav ol {
            flex-flow: wrap;
            place-content: space-around center;
            align-items: center;
            margin: 0;
            padding: 0;
            list-style-type: none;
            display: flex
        }

        header>nav ul li,
        header>nav ol li {
            display: inline-block
        }

        header>nav a,
        header>nav a:visited {
            border: 1px solid var(--border);
            border-radius: var(--standard-border-radius);
            color: var(--text);
            margin: 0 .5rem 1rem;
            padding: .1rem 1rem;
            text-decoration: none;
            display: inline-block
        }

        header>nav a:hover,
        header>nav a.current,
        header>nav a[aria-current=page] {
            border-color: var(--accent);
            color: var(--accent);
            cursor: pointer
        }

        @media only screen and (width<=720px) {
            header>nav a {
                border: none;
                padding: 0;
                line-height: 1;
                text-decoration: underline
            }
        }

        aside,
        details,
        pre,
        progress {
            background-color: var(--accent-bg);
            border: 1px solid var(--border);
            border-radius: var(--standard-border-radius);
            margin-bottom: 1rem
        }

        aside {
            float: right;
            width: 30%;
            margin-inline-start: 15px;
            padding: 0 15px;
            font-size: 1rem
        }

        [dir=rtl] aside {
            float: left
        }

        @media only screen and (width<=720px) {
            aside {
                float: none;
                width: 100%;
                margin-inline-start: 0
            }
        }

        article,
        fieldset,
        dialog {
            border: 1px solid var(--border);
            border-radius: var(--standard-border-radius);
            margin-bottom: 1rem;
            padding: 1rem
        }

        article h2:first-child,
        section h2:first-child,
        article h3:first-child,
        section h3:first-child {
            margin-top: 0.5rem;
            margin-bottom: 0.35rem;
        }

        section {
            border-top: 1px solid var(--border);
            border-bottom: 1px solid var(--border);
            margin: 3rem 0;
            padding: 2rem 1rem
        }

        section+section,
        section:first-child {
            border-top: 0;
            padding-top: 0
        }

        section+section {
            margin-top: 0
        }

        section:last-child {
            border-bottom: 0;
            padding-bottom: 0
        }

        details {
            padding: .7rem 1rem
        }

        summary {
            cursor: pointer;
            word-break: break-all;
            margin: -.7rem -1rem;
            padding: .7rem 1rem;
            font-weight: 700
        }

        details[open]>summary+* {
            margin-top: 0
        }

        details[open]>summary {
            margin-bottom: .5rem
        }

        details[open]>:last-child {
            margin-bottom: 0
        }

        table {
            width: 100%;
            border-collapse: collapse;
            margin: 1.5rem 0
        }

        figure>table {
            width: max-content;
            margin: 0
        }

        td,
        th {
            border: 1px solid var(--border);
            text-align: start;
            padding: .5rem
        }

        th {
            background-color: var(--accent-bg);
            font-weight: 700
        }

        tr:nth-child(2n) {
            background-color: var(--accent-bg)
        }

        table caption {
            margin-bottom: .5rem;
            font-weight: 700
        }

        textarea,
        select,
        input,
        button,
        .button {
            font-size: inherit;
            border-radius: var(--standard-border-radius);
            box-shadow: none;
            max-width: 100%;
            margin-bottom: .5rem;
            padding: .5rem;
            font-family: inherit;
            display: inline-block
        }

        textarea,
        select,
        input {
            color: var(--text);
            background-color: var(--bg);
            border: 1px solid var(--border)
        }

        label {
            display: block
        }

        textarea:not([cols]) {
            width: 100%
        }

        select:not([multiple]) {
            background-image: linear-gradient(45deg, transparent 49%, var(--text)51%), linear-gradient(135deg, var(--text)51%, transparent 49%);
            background-position: calc(100% - 15px), calc(100% - 10px);
            background-repeat: no-repeat;
            background-size: 5px 5px, 5px 5px;
            padding-inline-end: 25px
        }

        [dir=rtl] select:not([multiple]) {
            background-position: 10px, 15px
        }

        input[type=checkbox],
        input[type=radio] {
            vertical-align: middle;
            width: min-content;
            position: relative
        }

        input[type=checkbox]+label,
        input[type=radio]+label {
            display: inline-block
        }

        input[type=radio] {
            border-radius: 100%
        }

        input[type=checkbox]:checked,
        input[type=radio]:checked {
            background-color: var(--accent)
        }

        input[type=checkbox]:checked:after {
            content: " ";
            border-right: solid var(--bg).08em;
            border-bottom: solid var(--bg).08em;
            background-color: #0000;
            border-radius: 0;
            width: .18em;
            height: .32em;
            font-size: 1.8em;
            position: absolute;
            top: .05em;
            left: .17em;
            transform: rotate(45deg)
        }

        input[type=radio]:checked:after {
            content: " ";
            background-color: var(--bg);
            border-radius: 100%;
            width: .25em;
            height: .25em;
            font-size: 32px;
            position: absolute;
            top: .125em;
            left: .125em
        }

        @media only screen and (width<=720px) {

            textarea,
            select,
            input {
                width: 100%
            }
        }

        input[type=color] {
            height: 2.5rem;
            padding: .2rem
        }

        input[type=file] {
            border: 0
        }

        hr {
            background: var(--border);
            border: none;
            height: 1px;
            margin: 1rem auto
        }

        mark {
            border-radius: var(--standard-border-radius);
            background-color: var(--marked);
            color: #000;
            padding: 2px 5px
        }

        mark a {
            color: #0d47a1
        }

        img,
        video {
            border-radius: var(--standard-border-radius);
            max-width: 100%;
            height: auto
        }

        figure {
            margin: 0;
            display: block;
            overflow-x: auto
        }

        figure>img,
        figure>picture>img {
            margin-inline: auto;
            display: block
        }

        figcaption {
            text-align: center;
            color: var(--text-light);
            margin-block: 1rem;
            font-size: .9rem
        }

        blockquote {
            border-inline-start: .35rem solid var(--accent);
            color: var(--text-light);
            margin-block: 2rem;
            margin-inline: 2rem 0;
            padding: .4rem .8rem;
            font-style: italic
        }

        cite {
            color: var(--text-light);
            font-size: .9rem;
            font-style: normal
        }

        dt {
            color: var(--text-light)
        }

        code,
        pre,
        pre span,
        kbd,
        samp {
            font-family: var(--mono-font);
            color: var(--code)
        }

        kbd {
            color: var(--preformatted);
            border: 1px solid var(--preformatted);
            border-bottom: 3px solid var(--preformatted);
            border-radius: var(--standard-border-radius);
            padding: .1rem .4rem
        }

        pre {
            color: var(--preformatted);
            max-width: 100%;
            padding: 1rem 1.4rem;
            overflow: auto
        }

        pre code {
            color: var(--preformatted);
            background: 0 0;
            margin: 0;
            padding: 0
        }

        progress {
            width: 100%
        }

        progress:indeterminate {
            background-color: var(--accent-bg)
        }

        progress::-webkit-progress-bar {
            border-radius: var(--standard-border-radius);
            background-color: var(--accent-bg)
        }

        progress::-webkit-progress-value {
            border-radius: var(--standard-border-radius);
            background-color: var(--accent)
        }

        progress::-moz-progress-bar {
            border-radius: var(--standard-border-radius);
            background-color: var(--accent);
            transition-property: width;
            transition-duration: .3s
        }

        progress:indeterminate::-moz-progress-bar {
            background-color: var(--accent-bg)
        }

        dialog {
            max-width: 40rem;
            margin: auto
        }

        dialog::backdrop {
            background-color: var(--bg);
            opacity: .8
        }

        @media only screen and (width<=720px) {
            dialog {
                max-width: 100%;
                margin: auto 1em
            }
        }

        sup,
        sub {
            vertical-align: baseline;
            position: relative
        }

        sup {
            top: -.4em
        }

        sub {
            top: .3em
        }

        .notice {
            background: var(--accent-bg);
            border: 2px solid var(--border);
            border-radius: var(--standard-border-radius);
            margin: 2rem 0;
            padding: 1.5rem
        }
    </style>
    <style>
        article {
            margin-top: 0.5rem;
            margin-bottom: 2rem;
            border: 0.15rem solid var(--border);
            box-shadow: 0 0 1rem #000;
        }

        button,
        input {
            width: 100%;
        }

        .hint {
            margin: auto;
            text-align: center;
            font-style: italic;
            color: var(--text-light);
            font-size: 1rem;
        }

        .tr-btn:hover {
            background-color: var(--accent);
            color: var(--accent-text)
        }

        .btn-subtle {
            margin: 0.1rem;
            background-color: var(--accent-bg);
            color: var(--accent);
            border: 1px solid var(--accent);
            border-radius: var(--standard-border-radius);
            cursor: pointer;
        }

        .btn-subtle:hover {
            background-color: var(--accent);
            color: var(--accent-text);
        }

        .updategood {
            background: rgba(0, 122, 12, 0.247);
            border: 2px solid rgba(0, 122, 12, 0.935);
            border-radius: var(--standard-border-radius);
            margin: 2rem 0;
            padding: 1.5rem
        }

        .updatebad {
            background: rgba(122, 0, 0, 0.313);
            border: 2px solid rgba(122, 0, 0, 0.935);
            border-radius: var(--standard-border-radius);
            margin: 2rem 0;
            padding: 1.5rem
        }

        .update {
            background: var(--accent-bg);
            border: 2px solid var(--border);
            border-radius: var(--standard-border-radius);
            margin: 2rem 0;
            padding: 1.5rem
        }
    </style>
</head>

<body>
    <header>
        <h2>SLT DL Config for {UUID}</h2>
        <dt>Built by Hamza Anver (2024)</dt>
    </header>
    <p></p>
    <!--CONFIG CARDS-->
    <article>
        <h3>Access Point Configuration</h3>
        <form id="apcfg" onsubmit="fieldSetSendData(event,'apcfg','apcfg')">
            <label for="apalways">
                <input type="checkbox" id="apalways" name="apalways" checked /> Turn on AP always (even when connected
                to WiFi)
            </label>
            <label for="apssid">Access Point SSID</label>
            <input id="apssid" type="text" name="apssid" value="" placeholder="SSID" />
            <label for="appass">Access Point Password</label>
            <input id="appass" type="password" name="appass" value="" placeholder="Password (leave empty for none)" />
            <input type="checkbox" id="appass-toggle" name="appass-toggle" onclick="fieldSetTogglePassword('appass')" />
            Show Password
            <br>
            <button type="submit" name="connbtn">Save Access Point Config</button>
            <div id="apupdate">

            </div>
        </form>
    </article>
    <article>
        <h3> WiFi Configuration </h3>
        <table>
            <thead>
                <tr>
                    <th>SSID</th>
                    <th>RSSI</th>
                    <th>Security</th>
                </tr>
            </thead>
            <tbody id="stationnetworks-table">
                <tr>
                    <td colspan='3' class='hint'>No Networks to Show</td>
                </tr>
            </tbody>
            <tr>
                <td colspan='3'><button class="btn-subtle" onclick="StartWiFiScan();" id="stationscan-btn">Scan for WiFi
                        Networks</button></td>
            </tr>
        </table>
        <form id="stationcfg" onsubmit="fieldSetSendData(event,'stationcfg','stationcfg')">
            <label for="stationssid">Station SSID</label>
            <input id="stationssid" type="text" name="stationssid" value="" placeholder="SSID" />
            <label for="stationssid">Station Password</label>
            <input id="stationpass" type="password" name="stationpass" value="" placeholder="Password" />
            <input type="checkbox" id="stationpass-toggle" onclick="fieldSetTogglePassword('stationpass')">
            Show Password
            </input>
            <br>
            <br>
            <button type="submit" name="connbtn">Connect To Network</button>
        </form>
        <div id="stationupdate">

        </div>
    </article>
    <footer>
        example footer
    </footer>
    <script>
        // Field set functions
        function fieldSetTogglePassword(id) {
            var input = document.getElementById(id);
            var toggle = document.getElementById(id + "-toggle");
            if (input.type === "password") {
                input.type = "text";
                toggle.checked = true;
            } else {
                input.type = "password";
                toggle.checked = false;
            }
        }

        function fieldSetSendData(event, id, endpoint, updatestat_id = "") {
            event.preventDefault();
            var form = document.getElementById(id);
            var formData = new FormData(form);
            console.log(formData);
            var req = new XMLHttpRequest();
            req.open('POST', endpoint, true);
            req.onload = function () {
                if (this.readyState == 4 && this.status == 200) {
                    try {
                        var jsonResponse = JSON.parse(this.responseText);
                        console.log("Form JSON returned: ", jsonResponse);
                        fieldSetUpdate(jsonResponse);
                    } catch (error){
                        console.error("Form Submit Error: ", error)
                    }

                }
                else {
                    console.log("Error on Form Submit");
                }
            };
            req.onerror = function () {
                console.log("Error on Form Submit");
            };
            req.send(formData);
        }

        function fieldSetUpdate(jsonResponse) {
            if (jsonResponse.endpoint) {
                var req = new XMLHttpRequest();
                req.open('POST', jsonResponse.endpoint, true);
                req.onload = function () {
                    if (this.readyState == 4 && this.status == 200) {
                        let jsonResponse = JSON.parse(this.responseText);
                        console.log("Form update JSON parse: ", jsonResponse);
                        if (jsonResponse.updateid && jsonResponse.updatemsg) {
                            document.getElementById(jsonResponse.updateid).innerHTML = jsonResponse.updatemsg;
                        }

                        if (jsonResponse.timeout) {
                            setTimeout(function () {
                                fieldSetUpdate(jsonResponse);
                            }, jsonResponse.timeout);
                        }
                    }
                    else {
                        console.log("Error on Form Update");
                    }
                };
                req.onerror = function () {
                    console.log("Error on Form Update");
                };
                req.send(null);
            } else {
                if (jsonResponse.updatemsg && jsonResponse.updateid) {
                    console.log("Update form setting innerhtml of ["+ jsonResponse.updateid+ "] to ["+ jsonResponse.updatemsg+"]");
                    document.getElementById(jsonResponse.updateid).innerHTML = jsonResponse.updatemsg;
                }
            }
        }


        /*
        WIFI STATION CONFIGURATION
        */
        var WiFiScanBtn = document.getElementById("stationscan-btn");
        var wifi_networks = document.getElementById("stationnetworks-table");
        var ssid_input = document.getElementById("stationssid");

        function StartWiFiScan() {
            console.log("Sending Request for WiFi SSIDs");
            fetch("/startscan");
            // Do button disabling
            WiFiScanBtn.innerHTML = "Scanning...";
            WiFiScanBtn.disabled = true;
            wifi_networks.innerHTML = "<tr><td colspan='3' class='hint'>Scanning for networks...</td></tr>";
            setTimeout(GetWiFiNetworks, 5000);
        };

        function GetWiFiNetworks() {
            console.log("Getting WiFi Networks");
            var req = new XMLHttpRequest();
            req.open('GET', "/getscan", true);
            req.onload = function () {
                if (this.readyState == 4 && this.status == 200) {
                    var jsonResponse = JSON.parse(this.responseText);
                    console.log("WiFi Scan returned: ", jsonResponse);
                    if (jsonResponse.length > 1) {
                        wifi_networks.innerHTML = "";

                        for (var i = 1; i < jsonResponse.length; i++) {
                            var network_data = jsonResponse[i];
                            var new_row = "";
                            new_row += "<tr class='tr-btn'>";
                            new_row += "<td>" + network_data.ssid + "</td>";
                            new_row += "<td>" + network_data.rssi + "</td>";
                            new_row += "<td>" + network_data.encryption + "</td>";
                            new_row += "</tr>";
                            wifi_networks.innerHTML += new_row;
                        }
                        WiFiScanBtn.innerHTML = "Scan for WiFi Networks";
                        WiFiScanBtn.disabled = false;
                    } else {
                        setTimeout(GetWiFiNetworks, 1000);
                    }
                }
                else {
                    showError("submit", "Error on Refresh", true);
                }
            };
            req.onerror = function () {
                console.log("Error on Refresh..");
                showError("submit", "Error on Refresh", true);
            };
            req.send(null);
        };

        wifi_networks.addEventListener('click', (event) => {
            let target = event.target;
            // Traverse up the DOM to find the row element
            while (target && target.nodeName !== 'TR') {
                target = target.parentElement;
            }

            if (target && target.nodeName === 'TR') {
                console.log("WiFi Network Clicked: ", target.cells[0].textContent);
                ssid_input.value = target.cells[0].textContent;
            }
        });
    </script>
</body>

</html>)rawliteral";
