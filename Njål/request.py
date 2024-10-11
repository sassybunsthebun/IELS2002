# Første utkast til et python-script som skal kunne hente data fra API'et
# Første del av koden subscriber til et eller annet, vet ikke helt hva den gjør
# Andre del sender et request om posisjonsdata og lineRef, som gir en slags identifier 
# + lengdegrad og breddegrad til en hel masse busser. 
import requests
import socket

#-------------------SUBSCRIBE-------------------------------------------------------

# Define the URL where you are sending the request
# url = "https://api.entur.io/journey-planner/v3/graphql"
# url = "https://api.entur.io/realtime/v1/vehicles/graphql"
url = "https://api.entur.io/realtime/v1/services" # FUNGERENDE URL
# url = "wss://api.entur.io/realtime/v1/vehicles/subscriptions"


# Definerer XML dataen som en string (Generert av chat, usikker på hva mesteparten her gjør og om det er nødvendig)
xml_data = '''<?xml version="1.0" encoding="UTF-8"?>
<SubscriptionRequest>
  <RequestTimestamp>2024-09-30T12:00:00</RequestTimestamp>
  <RequestorRef> Njaalsinlaptop </RequestorRef>
  <MessageIdentifier> 123456 </MessageIdentifier>
  <InitialTerminationTime> 2024-09-30T18:00:00 </InitialTerminationTime>
  <VehicleMonitoringSubscriptionRequest>
    <LineRef>ATB:Line:2_980</LineRef>
  </VehicleMonitoringSubscriptionRequest>
</SubscriptionRequest>
'''

# headere henter fra: https://github.com/entur/api-examples/blob/master/journeyplanner/python/journeyplanner-response-to-json.py
headers = {'Accept': 'application/xml',
           'Content-Type': 'application/xml',
           'User-Agent': 'python-code-example-'+socket.gethostname(),
           'ET-Client-Name': 'python-code-example-'+socket.gethostname(),
           'ET-Client-ID': socket.gethostname()}

# header som ikke fungerer
'''
headers = {
  'Content-Type': 'application/json',
  'ET-Client-Name': 'Njaalslaptop'}
'''

# Sender POST request med XML data
response = requests.post(url, data=xml_data, headers=headers)

# Skjekker response
if response.status_code == 200:
    print("Subscription successful!")
    print(response.content)  # Printer responsen fra serveren
else:
    print(f"Error subscription failed: {response.status_code}")
    print(response.text) # printer feilmelding


# ----------------------------------REQUEST---------------------------------------

# GraphQL query definert som string, hentet fra: https://api.entur.io/graphql-explorer/vehicles?query=%0A%20%20%23%20Welcome%20to%20GraphiQL%0A%20%20%23%23>

query = '''
{
  vehicles(codespaceId:"ATB") {
    lastUpdated
    line {lineRef}
    location {
      latitude
      longitude
    }
  }
}
'''
# GraphQL query definert som string, hentet fra: https://developer.entur.org/pages-real-time-vehicle
# Denne queryen fungerer ikke
'''
query =
{
  subscription {
    vehicles(codespaceId:"ATB") {
      lastUpdated
      location {
        latitude
        longitude
      }
    }
  }
}
'''
# URL til GraphQL API endpoint
url = "https://api.entur.io/realtime/v1/vehicles/graphql"

# Headers til JSON, gjør egentlig ingenting
'''
headers_json = {
    'Content-Type': 'application/json',
    'ET-Client-Name': 'Njaalslaptop'
}
'''
# Make the POST request
response = requests.post(url, json={'query': query}, headers=headers)

# sjekker respons
if response.status_code == 200:
    print("Request successful!")
    print(response.json())  # Printer JSON respons
else:
    print(f"Error request failed: {response.status_code}")
    print(response.text)  # Printer feilmelding
