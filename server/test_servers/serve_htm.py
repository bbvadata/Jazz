#!/usr/bin/python

from flask import abort, Flask, jsonify, request, send_file


app = Flask(__name__)


countries = {'spain' : 'Madrid', 'france' : 'Paris', 'madagascar' : 'Antananarivo', 'malaysia' : 'Kuala Lumpur'}


@app.route('/test/str.blk', methods = ['GET'])
def get_file():
	return send_file('str.blk', mimetype = 'application/octet-stream')


@app.route('/test/str.blk', methods = ['PUT'])
def put_file():
	if not request.data:
		abort(400)

	return jsonify('Ok.')


@app.route('/test/capital/<string:country_id>', methods = ['GET'])
def get_capital(country_id):
	country_id = country_id.lower()

	if country_id in countries:
		return jsonify(countries[country_id])

	abort(404)


@app.route('/test/capital/<string:country_id>', methods = ['PUT'])
def put_capital(country_id):
	country_id = country_id.lower()

	if not request.data:
		abort(400)

	countries[country_id] = request.data.decode('utf-8')

	return jsonify('Ok.')


@app.route('/test/capital/<string:country_id>', methods = ['DELETE'])
def delete_capital(country_id):
	country_id = country_id.lower()

	if country_id in countries:
		del countries[country_id]
		return jsonify('Ok.')

	abort(404)


if __name__ == '__main__':
	app.run(debug = True)
