--- uboot/drivers/usbdcore.c	2006-11-02 09:15:01.000000000 -0500
+++ uboot/drivers/usbdcore.c	2012-03-23 10:26:53.704684371 -0400
@@ -594,14 +594,14 @@
  */
 void usbd_device_event_irq (struct usb_device_instance *device, usb_device_event_t event, int data)
 {
-	usb_device_state_t state;
+	//usb_device_state_t state;
 
 	if (!device || !device->bus) {
 		usberr("(%p,%d) NULL device or device->bus", device, event);
 		return;
 	}
 
-	state = device->device_state;
+	//state = device->device_state;
 
 	usbinfo("%s", usbd_device_events[event]);
 
