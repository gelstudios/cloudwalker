# cloudwalker

## API Endpoints

### Get Available Shoe IDs

Example: `curl api.iot.shoes`

* **Method**: `GET`
* **Success Response**:
  * **Code**: `200 OK`
  * **Example**:
      ```json
      [4,1,3,2]
      ```

### Change Shoe Color

Change the color on one or more shoes.

Example: `curl -d "shoeIds[]=1" -d "shoeIds[]=2" -d "color=blue" api.iot.shoes`

* **Method**: `POST`
* **Success Response**:
  * **Code**: `202 Accepted`
  * **Example**:
    ```json
    {"1":"color updated","2":"color updated"}
    ```
* **Error Response**:
  * **Code**: `400 Bad Request`
  * **Conditions**:
    * Not specifying one or more shoe Ids.
    * Not specifying a color.
    * One or more shoe Ids do not exist.
    * Invalid color.
