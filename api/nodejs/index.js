const express = require('express');
const bodyParser = require('body-parser');
const fs = require('fs').promises;
const urlencodedParser = bodyParser.urlencoded({ extended: true });
const colorString = require('color-string');

const app = express();
const port = 8000;
const path = './checkin';

/**
 * @return Promise
 */
function getShoeIds() {
  return fs.readdir(path);
}

/**
 * @param {number} id
 * @param {Array} color
 *   Ex: [255,0,0]
 * @return {Promise}
 */
function updateShoe(id, color) {
  const filename = `${path}/${id}`;
  const fileContents = color.splice(3).join(',');
  return fs.writeFile(filename, fileContents);
}

// returns shoe Ids
app.get('/', async function (req, res) {
  try {
    const shoeIds = await getShoeIds();
  } catch (e) {
    next(e);
  }

  res.send(`Available shoe Ids: ${shoeIds.join(', ')}`);
});

// change the color of one or more shoes
app.post('/', urlencodedParser, async function (req, res, next) {
  const { shoeIds, color } = req.body;

  if (!shoeIds || shoeIds.length === 0) {
    res.status(400).send('Please provide one or more shoe IDs.');
    return next();
  }

  if (!color) {
    res.status(400).send(`Please provide a color`);
    return next();
  }

  const colorAsRgb = colorString.get.rgb(color) || colorString.get.rgb(`rgb(${color})`);
  if (colorAsRgb === null) {
    res.status(400).send(`Unsupported color`);
    return next();
  }

  let allShoeIds = [];
  try {
    allShoeIds = await getShoeIds();
  } catch (e) {
    return next(e);
  }

  const intersection = shoeIds.filter(x => allShoeIds.indexOf(x) !== -1);
  if (intersection.length !== shoeIds.length) {
    res.status(400).send('One or more invalid shoe Ids');
    return next();
  }

  const result = [];
  for (id of shoeIds) {
    try {
      await updateShoe(id, colorAsRgb);
      result.push(`Shoe ${id} color updated`);
    } catch (e) {
      result.push(`Shoe ${id} color update failed`);
    }
  }

  res.status(202).json(result);
});

app.listen(port);
console.log(`Server is running on ${port}`);
