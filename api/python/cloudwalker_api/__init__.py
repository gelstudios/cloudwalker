#   Copyright 2019 Neal Gompa <ngompa13@gmail.com>
#   Fedora-License-Identifier: ASL 2.0
#   SPDX-2.0-License-Identifier: Apache-2.0
#   SPDX-3.0-License-Identifier: Apache-2.0
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import os
import pathlib

from flask import Flask, request, Response, jsonify, make_response

if not pathlib.Path("./checkin").is_dir():
    exit(f"Please create the following directory first: {pathlib.Path('./checkin')}")

app = Flask(__name__)


def get_shoe_ids() -> set:
    shoe_ids = set([])
    for checkin_file in os.listdir("./checkin"):
        if pathlib.Path(f"./checkin/{checkin_file}").is_file():
            shoe_ids.add(checkin_file)
    return shoe_ids


@app.route("/", methods=["GET", "POST"])
def main_app() -> Response:

    shoe_ids = get_shoe_ids()

    colors = {
        "red": (255, 0, 0),
        "green": (0, 255, 0),
        "blue": (0, 0, 255),
        "teal": (0, 255, 255),
        "purple": (255, 0, 255),
        "white": (255, 248, 255),
        "orange": (255, 16, 0),
        "pink": (255, 0, 32),
        "none": (0, 0, 0),
        "off": (0, 0, 0),
    }

    error_messages = {
        "missing_params": {
            "success": False,
            "message": "color and shoeIds are required parameters",
        },
        "shoeIds_not_list": {"success": False, "message": "shoeIds must be an array"},
        "invalid_shoeId": {"success": False, "message": "one or more invalid shoeIds"},
        "invalid_color": {
            "success": False,
            "message": f"color must be one of: {', '.join(colors.keys())}",
        },
    }

    if request.method == "GET":
        return make_response(jsonify(list(shoe_ids)), 200)
    if request.method == "POST":
        # Ensure we have necessary parameters
        # N.B.: We're being compatible with the PHP-style array pushing, but it's not a standard
        if not {"shoeIds[]", "color"}.issubset(set(request.form.keys())):
            return make_response(jsonify(error_messages["missing_params"]), 400)
        # Retrieve data
        req_shoe_ids = request.form.getlist("shoeIds[]")
        req_color = request.form.get("color")
        # Validate shoeId and color
        if type(req_shoe_ids) is not list:
            return make_response(jsonify(error_messages["shoeIds_not_list"]), 400)
        if not set(req_shoe_ids).issubset(set(shoe_ids)):
            return make_response(jsonify(error_messages["invalid_shoeId"]), 400)
        if req_color not in colors:
            return make_response(jsonify(error_messages["invalid_color"]), 400)
        # Set the shoes to the requested colors
        shoe_update_status = {}
        for shoe_id in req_shoe_ids:
            print(f"processing {shoe_id}")
            shoe_filepath = pathlib.Path(f"./checkin/{shoe_id}")
            try:
                shoe_file = open(shoe_filepath, "w")
                shoe_file.write(",".join(str(color_code) for color_code in colors[req_color]))
            except:
                shoe_update_status[shoe_id] = "color update failed"
            finally:
                shoe_file.close()
                shoe_update_status[shoe_id] = "color updated"
        return make_response(jsonify(shoe_update_status), 202)
